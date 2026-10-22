"""Apply a bounded WGSL fallthrough emission fix to a private Naga checkout.

Uses the same scoped duplication strategy as upstream Naga's HLSL writer.
The IR stays unchanged and goes through the normal validator. Never patches
the user's Cargo registry or another project's dependency.
"""
import hashlib
from pathlib import Path

ORIGINAL_SHA='fd35dab0482547dd984b8e726ed3f1f77d31a27c303e4c6a666240e1f211009e'


def patch(directory):
    path=Path(directory)/'src/back/wgsl/writer.rs'
    data=path.read_bytes()
    if b'// SSBU scoped fallthrough expansion' in data:return
    if hashlib.sha256(data).hexdigest()!=ORIGINAL_SHA:
        raise ValueError('Unexpected Naga 29.0.4 WGSL writer checksum')
    source=data.decode()
    start=source.index('                for case in cases {\n                    if case.fall_through')
    end=source.index('\n                    match case.value {',start)
    source=source[:start]+'''                for (case_index, case) in cases.iter().enumerate() {
                    // SSBU scoped fallthrough expansion
                    let expand = case.fall_through && !case.body.is_empty();
                    let fall_through = case.fall_through && !expand;
'''+source[end:]
    end=source.index('\n                writeln!(self.out, "{level}}}")?',start)
    segment=source[start:end]
    segment=segment.replace('new_case = !case.fall_through','new_case = !fall_through')
    segment=segment.replace('if case.fall_through {','if fall_through {')
    segment=segment.replace('if !case.fall_through {','if !fall_through {')
    old='''                    for sta in case.body.iter() {
                        self.write_stmt(module, sta, func_ctx, l2.next())?;
                    }'''
    new='''                    if expand {
                        let mut expanded = 0usize;
                        for source in &cases[case_index..] {
                            expanded += source.body.len();
                            if expanded > 1_000_000 {
                                return Err(Error::Unimplemented("switch expansion budget".into()));
                            }
                            writeln!(self.out, "{}{{", l2.next())?;
                            let previous = self.named_expressions.len();
                            for sta in source.body.iter() {
                                self.write_stmt(module, sta, func_ctx, l2.next().next())?;
                            }
                            self.named_expressions.truncate(previous);
                            writeln!(self.out, "{}}}", l2.next())?;
                            if !source.fall_through { break; }
                        }
                    } else {
                        for sta in case.body.iter() {
                            self.write_stmt(module, sta, func_ctx, l2.next())?;
                        }
                    }'''
    if old not in segment:raise ValueError('Naga case body changed')
    source=source[:start]+segment.replace(old,new,1)+source[end:]
    path.write_text(source,encoding='utf-8',newline='\n')
