// MallVR: drop the dev-PC-only test player from shipped source (same rules as package/make_package.py strip_dev_only): the
// "#if defined(__has_include)" block that pulls in vpix_testbot.h and every "#ifdef VPIX_TESTBOT" section (an #else part is kept).
//   node strip_dev_only.js <file> [<file> ...]      edits the files in place (use on COPIES); exits 1 if "testbot" is still in one
const fs = require('fs');
let bad = false;
for (const path of process.argv.slice(2)) {
    const raw = fs.readFileSync(path, 'utf8'), nl = raw.includes('\r\n') ? '\r\n' : '\n';
    const out = []; let mode = null, depth = 0;
    for (const line of raw.split(nl)) {
        const s = line.trim();
        if (mode === null && s === '#if defined(__has_include)') { mode = 'skip'; depth = 1; continue; }
        if (mode === null && s === '#ifdef VPIX_TESTBOT') { mode = 'skip_dev'; depth = 1; continue; }
        if (mode !== null) {
            if (s.startsWith('#if')) depth++;
            else if (s === '#else' && mode === 'skip_dev' && depth === 1) { mode = 'keep_else'; continue; }
            else if (s === '#endif') { depth--; if (depth === 0) { mode = null; continue; } }
            if (mode === 'keep_else') out.push(line);
            continue;
        }
        out.push(line);
    }
    const text = out.join(nl);
    if (/testbot/i.test(text) || text.includes('VPIX_TESTPOSE')) { console.error('shipped source still mentions the test player: ' + path); bad = true; }
    fs.writeFileSync(path, text);
}
process.exit(bad ? 1 : 0);
