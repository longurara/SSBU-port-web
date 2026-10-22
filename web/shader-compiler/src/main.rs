fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<_> = std::env::args().collect();
    if args.len()==2 && args[1]=="--version" { println!("ssbu-web-shader 0.1.0 (naga 29.0.4)"); return Ok(()); }
    if args.len()!=3 { return Err("usage: ssbu-web-shader input.spv output.wgsl".into()); }
    let result=ssbu_web_shader::convert(&std::fs::read(&args[1])?)?;
    std::fs::write(&args[2],result)?;
    Ok(())
}
