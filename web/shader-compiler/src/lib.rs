//! Browser-owned SPIR-V -> WGSL compiler. No native service is required.
//! Unsupported shader features return diagnostics; no placeholder shader.
#[cfg(test)]
use naga::Statement;
use std::cell::RefCell;

pub fn convert(bytes: &[u8]) -> Result<String, String> {
    convert_with_bindings(bytes).map(|(code,_)|code)
}
fn active_bindings(module:&naga::Module,info:&naga::valid::ModuleInfo)->String {
    let entries:Vec<String>=module.global_variables.iter().filter_map(|(handle,global)|{
        let binding=global.binding.as_ref()?;
        if !(0..module.entry_points.len()).any(|index|!info.get_entry_point(index)[handle].is_empty()){return None;}
        Some(format!("[{},{}]",binding.group,binding.binding))
    }).collect();
    format!("[{}]",entries.join(","))
}
fn convert_with_bindings(bytes: &[u8]) -> Result<(String,String), String> {
    if bytes.len() > 16 * 1024 * 1024 { return Err("SPIR-V exceeds 16 MiB".into()); }
    let options = naga::front::spv::Options {
        // The renderer must handle viewport conventions explicitly. Do not
        // apply a hidden second flip to shaders from the native Vulkan path.
        adjust_coordinate_space: false,
        strict_capabilities: true,
        block_ctx_dump_prefix: None,
    };
    let module = naga::front::spv::parse_u8_slice(bytes, &options)
        .map_err(|e| e.to_string())?;
    let info = naga::valid::Validator::new(
        // These are core WGSL pack/unpack/quantize builtins operating on f32.
        // This capability does not enable f16 types or require shader-f16.
        naga::valid::ValidationFlags::all(), naga::valid::Capabilities::default() | naga::valid::Capabilities::SHADER_FLOAT16_IN_FLOAT32)
        .validate(&module).map_err(|e| format!("{e:?}"))?;
    let code=naga::back::wgsl::write_string(&module, &info, naga::back::wgsl::WriterFlags::EXPLICIT_TYPES)
        .map_err(|e| e.to_string())?;
    Ok((code,active_bindings(&module,&info)))
}

thread_local! {
    static INPUT: RefCell<Vec<u8>> = const { RefCell::new(Vec::new()) };
    static OUTPUT: RefCell<Vec<u8>> = const { RefCell::new(Vec::new()) };
    static BINDINGS: RefCell<Vec<u8>> = const { RefCell::new(Vec::new()) };
}

// Each Worker owns one instance. Pointers are valid until the next call that
// replaces that buffer; JS copies output before reusing it.
#[no_mangle]
pub extern "C" fn shader_input(size: usize) -> *mut u8 {
    if size > 16 * 1024 * 1024 { return std::ptr::null_mut(); }
    INPUT.with(|slot| { let mut v = slot.borrow_mut(); v.resize(size, 0); v.as_mut_ptr() })
}
#[no_mangle]
pub extern "C" fn shader_convert() -> u32 {
    let result = INPUT.with(|slot| convert_with_bindings(&slot.borrow()));
    let (status, text, bindings) = match result { Ok((text,bindings)) => (0, text, bindings), Err(text) => (1, text, "[]".into()) };
    OUTPUT.with(|slot| *slot.borrow_mut() = text.into_bytes());
    BINDINGS.with(|slot| *slot.borrow_mut() = bindings.into_bytes());
    status
}
#[no_mangle]
pub extern "C" fn shader_output() -> *const u8 { OUTPUT.with(|slot| slot.borrow().as_ptr()) }
#[no_mangle]
pub extern "C" fn shader_output_size() -> usize { OUTPUT.with(|slot| slot.borrow().len()) }
#[no_mangle]
pub extern "C" fn shader_bindings() -> *const u8 { BINDINGS.with(|slot|slot.borrow().as_ptr()) }
#[no_mangle]
pub extern "C" fn shader_bindings_size() -> usize { BINDINGS.with(|slot|slot.borrow().len()) }

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn copies_fallthrough_chains_and_preserves_breaks() {
        let mut module = naga::front::wgsl::parse_str("fn next(x:u32)->u32 { return x+1u; } @compute @workgroup_size(1) fn main() { var x=0u; switch x { case 0u: { x=next(7u); } case 1u: { x=next(9u); } default: { x=next(11u); } } }").unwrap();
        let function = &mut module.entry_points[0].function;
        let switch = function.body.iter_mut().find_map(|s| match s { Statement::Switch {cases,..} => Some(cases), _=>None }).unwrap();
        switch[0].fall_through = true;
        switch[1].fall_through = true;
        let info=naga::valid::Validator::new(naga::valid::ValidationFlags::all(),naga::valid::Capabilities::default()).validate(&module).unwrap();
        let text=naga::back::wgsl::write_string(&module,&info,naga::back::wgsl::WriterFlags::EXPLICIT_TYPES).unwrap();
        // Reparse and validate emitted WGSL, catching scope and emission errors.
        let output=naga::front::wgsl::parse_str(&text).unwrap();
        naga::valid::Validator::new(naga::valid::ValidationFlags::all(),naga::valid::Capabilities::default()).validate(&output).unwrap();
        assert_eq!(text.matches("11u").count(),3);
        assert_eq!(text.matches("9u").count(),2);
    }

    #[test]
    fn rejects_oversized_input() {
        assert!(convert(&vec![0u8;16*1024*1024+1]).is_err());
    }
    #[test]
    fn malformed_input_returns_error() { assert!(convert(b"invalid").is_err()); }
    #[test]
    fn reflects_only_resources_used_by_entry_call_graph() {
        let module=naga::front::wgsl::parse_str("@group(0) @binding(41) var<storage,read> used:array<u32>; @group(0) @binding(42) var<storage,read> unused:array<u32>; @group(0) @binding(43) var<storage,read_write> output:array<u32>; fn helper()->u32{return used[0];} @compute @workgroup_size(1) fn main(){output[0]=helper();}").unwrap();
        let info=naga::valid::Validator::new(naga::valid::ValidationFlags::all(),naga::valid::Capabilities::default()).validate(&module).unwrap();
        assert_eq!(active_bindings(&module,&info),"[[0,41],[0,43]]");
    }
}
