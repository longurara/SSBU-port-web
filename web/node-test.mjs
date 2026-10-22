import { readFile, writeFile } from 'node:fs/promises';

const manifest = JSON.parse(await readFile(new URL('./manifest.json', import.meta.url), 'utf8'));
const report = { environment: 'node', node: process.version, scope: manifest.scope, results: {}, errors: {} };
for (const target of manifest.targetsBuiltThisInvocation) {
  const records = [];
  let finish, fail;
  const done = new Promise((resolve, reject) => { finish = resolve; fail = reject; });
  done.catch(() => {});
  const deadline = setTimeout(() => fail(new Error(`${target} timed out`)), 20000);
  try {
    const { default: create } = await import(`./${target}.mjs`);
    await create({
      print(line) {
        if (line.startsWith('SSBU_')) {
          const split = line.indexOf(' ');
          const kind = line.slice(0, split), value = JSON.parse(line.slice(split + 1));
          records.push({ kind, ...value });
          if (kind === 'SSBU_CHECK' && !value.ok) console.error(`${target}: FAIL ${value.name}`);
          if (kind.endsWith('_RESULT')) finish(value);
        }
      },
      printErr: line => console.error(`${target}: ${line}`),
      onAbort: reason => fail(new Error(`${target}: ${reason}`)),
    });
    const result = await done;
    report.results[target] = { ...result, records };
    const expectedBits = target.endsWith('64') ? 64 : 32;
    if (result.failures !== 0 || result.pointerBits !== expectedBits || (target.startsWith('core') && !result.checks) || (target.startsWith('game') && result.checks !== manifest.game?.cases)) throw new Error(`${target} failed: ${JSON.stringify(result)}`);
    console.log(`${target}: PASS (${result.checks ?? 'fiber'} checks, pointer ${result.pointerBits})`);
  } catch (error) {
    report.results[target] ??= { records };
    report.errors[target] = String(error);
    console.error(error);
  } finally { clearTimeout(deadline); }
}
if (report.results.core32?.checks !== report.results.core64?.checks) report.errors.parity = '32/64 check counts differ';
report.ok = Object.keys(report.errors).length === 0;
await writeFile(new URL('./node-report.json', import.meta.url), JSON.stringify(report, null, 2));
console.log('Node report: node-report.json');
if (!report.ok) process.exitCode = 1;
