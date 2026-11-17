// Paste into the page of mixamo.com (logged in) through the browser's JavaScript tool: defines __mxMany, which exports
// clips (FBX, without skin, 30 fps) and returns their signed addresses (valid for 300 seconds).
// Then run Tools/Blender/mixamo_fetch_exports.py <folder>: it reads the addresses from the session's transcript and
// saves the files (the page itself can't reach localhost, and its own Download button opens a save dialog).
// Example: (await __mxMany([['mutant', 'Mutant Roaring'], ['boxing', 'Boxing Idle']])).join('\n')
//          each pair is [search words, the exact description of the clip]. __mxSearch('boxing') lists descriptions.
// The user allowed this on 2026-11-16 (the session's permission check asks otherwise).
window.__mxChar = '90815396-6b00-4efc-b670-4c3497dbb605';   // the character open in the user's Mixamo (any Mixamo rig works)
window.__mxHeaders = () => ({'Authorization': 'Bearer ' + localStorage.access_token, 'X-Api-Key': 'mixamo2', 'Accept': 'application/json', 'Content-Type': 'application/json'});
window.__mxSearch = async (q) => { const r = await fetch('https://www.mixamo.com/api/v1/products?page=1&limit=96&order=&type=Motion&query=' + encodeURIComponent(q), {headers: __mxHeaders()}); const j = await r.json(); return j.results.map(x => x.name + ' | ' + x.description); };
window.__mxExport = async (query, description) => {
  const h = __mxHeaders();
  const s = await (await fetch('https://www.mixamo.com/api/v1/products?page=1&limit=96&order=&type=Motion&query=' + encodeURIComponent(query), {headers: h})).json();
  const hit = s.results.find(x => x.description === description);
  if (!hit) return 'NOT FOUND ' + description;
  const d = await (await fetch('https://www.mixamo.com/api/v1/products/' + hit.id + '?similar=0&character_id=' + __mxChar, {headers: h})).json();
  const gms = d.details.gms_hash;
  gms.params = gms.params.map(p => p[1]).join(',');
  gms.overdrive = 0;
  const body = {character_id: __mxChar, gms_hash: [gms], preferences: {format: 'fbx7_2019', skin: 'false', fps: '30', reducekf: '0'}, product_name: d.name, type: 'Motion'};
  const e = await fetch('https://www.mixamo.com/api/v1/animations/export', {method: 'POST', headers: h, body: JSON.stringify(body)});
  if (!e.ok) return 'EXPORT FAILED ' + e.status;
  for (let i = 0; i < 40; i++) {
    await new Promise(r => setTimeout(r, 2000));
    const m = await (await fetch('https://www.mixamo.com/api/v1/characters/' + __mxChar + '/monitor', {headers: h})).json();
    if (m.status === 'completed') return m.job_result;
    if (m.status === 'failed') return 'JOB FAILED';
  }
  return 'TIMEOUT';
};
window.__mxMany = async (list) => { const out = []; for (const [q, d] of list) out.push(await __mxExport(q, d)); return out; };
