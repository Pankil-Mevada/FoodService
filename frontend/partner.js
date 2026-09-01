const storageKey='plated_partner_draft_v1',token=localStorage.getItem('plated_token')||'';
const blank={restaurant:{name:'',phone:'',address:'',radius:8,latitude:'',longitude:''},items:[],audit:[]};
let state;try{state={...blank,...JSON.parse(localStorage.getItem(storageKey)||'{}')}}catch{state=blank}
const $=s=>document.querySelector(s),$$=s=>[...document.querySelectorAll(s)];
const esc=v=>{const d=document.createElement('div');d.textContent=v;return d.innerHTML};
function record(action){state.audit.unshift({action,at:new Date().toISOString()});state.audit=state.audit.slice(0,50)}
function save(){localStorage.setItem(storageKey,JSON.stringify(state));render()}
function render(){const f=$('#restaurant-form');Object.entries(state.restaurant).forEach(([k,v])=>{if(f.elements[k])f.elements[k].value=v});$('#item-count').textContent=state.items.length;
 $('#items').innerHTML=state.items.length?state.items.map((x,i)=>`<div class="menu-item"><span><b>${esc(x.name)}</b><br>${x.diet}</span><span>₹${x.price.toFixed(2)} <button data-remove="${i}">Remove</button></span></div>`).join(''):'<div class="card">Add the first menu item. Production stores money as integer paise.</div>';
 $$('[data-remove]').forEach(b=>b.onclick=()=>{state.items.splice(+b.dataset.remove,1);record('MENU_ITEM_REMOVED');save()});
 $('#audit-list').innerHTML=state.audit.length?state.audit.map(x=>`<div class="audit-row"><b>${x.action}</b><span>${new Date(x.at).toLocaleString()}</span></div>`).join(''):'<div class="card">No draft activity yet.</div>'}
$$('[data-view]').forEach(b=>b.onclick=()=>{$$('[data-view]').forEach(x=>x.classList.toggle('active',x===b));$$('[data-panel]').forEach(p=>p.hidden=p.dataset.panel!==b.dataset.view)});
$('#restaurant-form').onsubmit=e=>{e.preventDefault();state.restaurant=Object.fromEntries(new FormData(e.currentTarget));record('RESTAURANT_DRAFT_UPDATED');save();$('#notice').textContent='Browser preview saved. Publishing is blocked until the authenticated partner API and approval workflow are connected.'};
$('#item-form').onsubmit=e=>{e.preventDefault();const d=Object.fromEntries(new FormData(e.currentTarget)),price=Number(d.price);if(!Number.isFinite(price)||price<0)return;state.items.push({name:String(d.name).trim(),price,diet:d.diet});record('MENU_ITEM_CREATED');e.currentTarget.reset();save()};
$('#signout').onclick=()=>{localStorage.removeItem('plated_token');location.href='index.html'};
if(!token)$('#notice').textContent='Sign in first. Browser storage is never partner authorization; production writes require JWT plus server-side membership checks.';
render();
