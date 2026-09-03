'use strict';
const config={apiUrl:localStorage.getItem('plated_api_url')||'http://localhost:8085'};
const state={token:localStorage.getItem('plated_partner_token')||'',authMode:'login',identity:null,restaurants:[],selected:null,items:[],orders:[],audit:[],pendingCommands:new Map()};
const $=s=>document.querySelector(s),$$=s=>[...document.querySelectorAll(s)];
const esc=value=>String(value??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));

function notice(message,type=''){const box=$('#notice');box.textContent=message;box.className=type}
async function request(path,options={}){
 const headers={Accept:'application/json',...(options.body?{'Content-Type':'application/json'}:{}),...(options.headers||{})};
 if(state.token)headers.Authorization='Bearer '+state.token;
 let response;try{response=await fetch(config.apiUrl+path,{...options,headers})}catch{throw new Error('Cannot reach the API gateway')}
 const text=await response.text();let data={};try{data=text?JSON.parse(text):{}}catch{data={message:text}}
 if(!response.ok){const error=new Error(data.message||'Request failed ('+response.status+')');error.status=response.status;throw error}return data;
}
function showAuth(show){$('#auth-panel').hidden=!show;$('#app-shell').hidden=show;$('#signout').hidden=show}
function activeId(){return state.selected?.id||0}
function editable(){return ['DRAFT','REJECTED'].includes(String(state.selected?.status||''))}
function showPanel(name){$$('[data-view]').forEach(x=>x.classList.toggle('active',x.dataset.view===name));$$('[data-panel]').forEach(x=>x.hidden=x.dataset.panel!==name)}

async function authenticate(){
 if(!state.token){showAuth(true);return}
 try{
  state.identity=await request('/me');$('#identity').textContent=state.identity.email||state.identity.name||'Partner';
  showAuth(false);await loadRestaurants();
 }catch(error){state.token='';localStorage.removeItem('plated_partner_token');showAuth(true);notice(error.message,'error')}
}
async function loadRestaurants(){
 state.restaurants=await request('/partner/restaurants');const picker=$('#restaurant-select');picker.disabled=false;
 picker.innerHTML='<option value="">Create a restaurant</option>'+state.restaurants.map(r=>'<option value="'+r.id+'">'+esc(r.name)+' · '+esc(r.status)+'</option>').join('');
 const remembered=Number(sessionStorage.getItem('plated_partner_restaurant')||0);
 selectRestaurant(state.restaurants.find(r=>r.id===remembered)||state.restaurants[0]||null);
}
async function selectRestaurant(restaurant){
 state.selected=restaurant;$('#restaurant-select').value=restaurant?.id||'';if(restaurant)sessionStorage.setItem('plated_partner_restaurant',restaurant.id);else sessionStorage.removeItem('plated_partner_restaurant');
 fillRestaurant();await Promise.all([loadItems(),loadOrders(),loadAudit()]);renderSummary();
}
function fillRestaurant(){
 const form=$('#restaurant-form'),r=state.selected||{};
 ['name','phone','address','latitude','longitude','deliveryRadiusKm','preparationMinutes','baseDeliveryFee','perKmFee','imageUrl'].forEach(name=>{form.elements[name].value=r[name]??({deliveryRadiusKm:8,preparationMinutes:20,baseDeliveryFee:39,perKmFee:5}[name]??'')});
 [...form.elements].forEach(element=>{if(element.tagName!=='BUTTON')element.disabled=Boolean(state.selected&&!editable())});
}
function renderSummary(){
 $('#status').textContent=state.selected?.status||'NO RESTAURANT';$('#role').textContent=state.selected?.role||'—';$('#item-count').textContent=state.items.length;
 $('#submit-review').disabled=!state.selected||!editable()||state.items.length===0;
}
async function loadItems(){
 state.items=state.selected?await request('/partner/restaurants/'+activeId()+'/menu-items'):[];renderItems();
}
function renderItems(){
 const box=$('#items');if(!state.selected){box.innerHTML='<div class="empty">Create a restaurant before adding menu items.</div>';return}
 if(!state.items.length){box.innerHTML='<div class="empty">No menu items yet. Add one before submitting for review.</div>';return}
 box.innerHTML=state.items.map(item=>'<article class="menu-item"><div><b>'+esc(item.name)+'</b><small>'+esc(item.description||'No description')+' · '+esc(item.dietType)+'</small></div><div class="menu-actions"><b>₹'+(Number(item.pricePaise)/100).toFixed(2)+'</b><button class="danger remove-item" data-id="'+item.id+'">Remove</button></div></article>').join('');
 $$('.remove-item').forEach(button=>button.onclick=()=>removeItem(Number(button.dataset.id)));
}
function nextOrderAction(order){
 const actions={NEW:['ACCEPTED','Accept order'],ACCEPTED:['PREPARING','Start preparing'],PREPARING:['READY_FOR_PICKUP','Mark ready'],READY_FOR_PICKUP:['HANDED_OFF','Confirm handoff']};
 return actions[order.restaurantStatus]||null;
}
async function loadOrders(){
 const box=$('#partner-orders');
 if(!state.selected){state.orders=[];box.innerHTML='<div class="empty">Choose or create a restaurant to load its paid orders.</div>';return}
 box.innerHTML='<div class="empty">Loading paid orders…</div>';
 try{state.orders=await request('/partner/restaurants/'+activeId()+'/orders');renderOrders()}
 catch(error){state.orders=[];box.innerHTML='<div class="empty error-state"><b>Could not load orders</b><span>'+esc(error.message)+'</span><button type="button" id="retry-orders">Retry</button></div>';$('#retry-orders').onclick=loadOrders}
}
function renderOrders(){
 const box=$('#partner-orders');
 if(!state.orders.length){box.innerHTML='<div class="empty"><b>No paid orders waiting</b><span>New orders appear after payment is verified.</span></div>';return}
 box.innerHTML=state.orders.map(order=>{const action=nextOrderAction(order),handoffBlocked=action&&action[0]==='HANDED_OFF'&&!order.driverAssigned;return '<article class="partner-order '+(order.restaurantStatus==='NEW'?'new-order':'')+'"><div class="order-top"><div><span class="order-number">ORDER #'+order.id+'</span><h3>'+esc(order.itemSummary||'Order items unavailable')+'</h3></div><span class="kitchen-status">'+esc(order.restaurantStatus.replaceAll('_',' '))+'</span></div><p class="delivery-address">Deliver to · '+esc(order.deliveryAddress||'Address unavailable')+'</p><div class="order-meta"><span><small>TOTAL</small><b>₹'+Number(order.totalAmount).toFixed(2)+'</b></span><span><small>DELIVERY</small><b>'+esc(order.orderStatus.replaceAll('_',' '))+'</b></span><span><small>PREP TIME</small><b>'+Number(order.preparationMinutes)+' min</b></span></div>'+(action?'<div class="order-action">'+(action[0]==='ACCEPTED'?'<label>Preparation minutes<input class="prep-minutes" data-order-id="'+order.id+'" type="number" min="1" max="240" value="'+Number(order.preparationMinutes||20)+'"></label>':'')+'<button type="button" class="primary advance-order" data-order-id="'+order.id+'" '+(handoffBlocked?'disabled title="A driver must be assigned first"':'')+'>'+action[1]+'</button>'+(handoffBlocked?'<small>Waiting for driver assignment before handoff.</small>':'')+'</div>':'<p class="complete-state">Kitchen workflow complete · handed to delivery</p>')+'</article>'}).join('');
 $$('.advance-order').forEach(button=>button.onclick=()=>advanceOrder(Number(button.dataset.orderId),button));
}
async function advanceOrder(orderId,button){
 const order=state.orders.find(value=>value.id===orderId),action=order&&nextOrderAction(order);if(!action)return;
 const commandKey=orderId+':'+action[0];let idempotencyKey=state.pendingCommands.get(commandKey);if(!idempotencyKey){idempotencyKey=crypto.randomUUID();state.pendingCommands.set(commandKey,idempotencyKey)}
 const body={status:action[0],expectedVersion:order.version};if(action[0]==='ACCEPTED')body.preparationMinutes=Number($('.prep-minutes[data-order-id="'+orderId+'"]')?.value||20);
 button.disabled=true;
 try{await request('/partner/restaurants/'+activeId()+'/orders/'+orderId+'/status',{method:'POST',body:JSON.stringify(body),headers:{'Idempotency-Key':idempotencyKey}});state.pendingCommands.delete(commandKey);notice('Order #'+orderId+' moved to '+action[0].replaceAll('_',' ')+'.','success');await loadOrders()}
 catch(error){if(error.status===409){state.pendingCommands.delete(commandKey);await loadOrders()}notice(error.message,'error');button.disabled=false}
}
async function loadAudit(){
 state.audit=state.selected?await request('/partner/restaurants/'+activeId()+'/audit'):[];const box=$('#audit-list');
 box.innerHTML=state.audit.length?state.audit.map(event=>'<div class="audit-row"><b>'+esc(event.action)+'</b><span>User '+event.actorUserId+' · '+new Date(Number(event.createdEpoch)*1000).toLocaleString()+'</span></div>').join(''):'<div class="empty">No server activity for this restaurant.</div>';
}
async function refreshSelected(){
 if(!activeId())return;const updated=await request('/partner/restaurants/'+activeId());state.restaurants=state.restaurants.map(r=>r.id===updated.id?updated:r);await selectRestaurant(updated);
}

function setAuthMode(mode){
 state.authMode=mode;const register=mode==='register',form=$('#partner-auth-form');
 $$('[data-auth-mode]').forEach(button=>{const active=button.dataset.authMode===mode;button.classList.toggle('active',active);button.setAttribute('aria-selected',String(active))});
 $$('.register-field').forEach(element=>element.hidden=!register);form.elements.name.required=register;form.elements.password.autocomplete=register?'new-password':'current-password';
 $('#auth-title').textContent=register?'Create your restaurant partner account.':'Sign in to your restaurant workspace.';
 $('#auth-description').textContent=register?'Create the identity used to own and manage restaurants. You will stay on the partner portal.':'Your partner session stays in this portal. Identity is verified by User Service; restaurant access is enforced separately by Restaurant Service.';
 $('button.primary',form).textContent=register?'Create partner account':'Sign in securely';notice('');
}
$$('[data-auth-mode]').forEach(button=>button.onclick=()=>setAuthMode(button.dataset.authMode));
$('#partner-auth-form').onsubmit=async event=>{
 event.preventDefault();const form=event.currentTarget,button=$('button.primary',form),body=Object.fromEntries(new FormData(form));body.email=String(body.email||'').trim().toLowerCase();
 if(state.authMode==='login')delete body.name;else if(!/^(?=.*[a-z])(?=.*[A-Z])(?=.*\d)(?=.*[^A-Za-z0-9]).{10,128}$/.test(String(body.password||''))){notice('Use 10+ characters with uppercase, lowercase, a number, and a symbol','error');return}
 button.disabled=true;
 try{
  if(state.authMode==='register'){await request('/register',{method:'POST',body:JSON.stringify(body)});notice('Partner account created. Signing you in…','success')}
  const credentials={email:body.email,password:body.password},result=await request('/login',{method:'POST',body:JSON.stringify(credentials)});
  if(!result.token)throw new Error('Login did not return a token');state.token=result.token;localStorage.setItem('plated_partner_token',result.token);form.reset();notice('Signed in. Loading your restaurants…','success');await authenticate();
 }catch(error){notice(error.message,'error')}finally{button.disabled=false}
};
$('#signout').onclick=()=>{state.token='';localStorage.removeItem('plated_partner_token');sessionStorage.removeItem('plated_partner_restaurant');state.selected=null;showAuth(true);$('#identity').textContent='Not signed in';setAuthMode('login');notice('Signed out','success')};
$('#restaurant-select').onchange=()=>selectRestaurant(state.restaurants.find(r=>r.id===Number($('#restaurant-select').value))||null);
$('#refresh-orders').onclick=loadOrders;
$('#new-restaurant').onclick=()=>{selectRestaurant(null);showPanel('restaurant');notice('Enter the restaurant details. Saving creates a private DRAFT.','success')};
$$('[data-view]').forEach(button=>button.onclick=()=>showPanel(button.dataset.view));

$('#restaurant-form').onsubmit=async event=>{
 event.preventDefault();const form=event.currentTarget,button=$('button',form),data=Object.fromEntries(new FormData(form));
 const body={name:data.name.trim(),phone:data.phone.trim(),address:data.address.trim(),latitude:Number(data.latitude),longitude:Number(data.longitude),deliveryRadiusKm:Number(data.deliveryRadiusKm),preparationMinutes:Number(data.preparationMinutes),baseDeliveryFee:Number(data.baseDeliveryFee),perKmFee:Number(data.perKmFee),imageUrl:data.imageUrl.trim()};
 if(state.selected)body.version=state.selected.version;button.disabled=true;
 try{
  const path=state.selected?'/partner/restaurants/'+activeId():'/partner/restaurants';
  const result=await request(path,{method:state.selected?'PUT':'POST',body:JSON.stringify(body)});
  notice(state.selected?'Restaurant saved in Restaurant Service':'Private restaurant draft created','success');
  if(!state.selected)sessionStorage.setItem('plated_partner_restaurant',result.restaurantId);await loadRestaurants();
 }catch(error){notice(error.message,'error')}finally{button.disabled=false}
};
$('#item-form').onsubmit=async event=>{
 event.preventDefault();if(!state.selected){notice('Create a restaurant first','error');return}
 const form=event.currentTarget,button=$('button',form),data=Object.fromEntries(new FormData(form));
 const body={name:data.name.trim(),description:data.description.trim(),pricePaise:Math.round(Number(data.price)*100),dietType:data.dietType,available:true};button.disabled=true;
 try{await request('/partner/restaurants/'+activeId()+'/menu-items',{method:'POST',body:JSON.stringify(body),headers:{'Idempotency-Key':crypto.randomUUID()}});form.reset();notice('Menu item saved in Restaurant Service','success');await Promise.all([loadItems(),loadAudit()]);renderSummary()}catch(error){notice(error.message,'error')}finally{button.disabled=false}
};
async function removeItem(itemId){
 if(!confirm('Remove this menu item?'))return;
 try{await request('/partner/restaurants/'+activeId()+'/menu-items/'+itemId,{method:'DELETE'});notice('Menu item removed','success');await Promise.all([loadItems(),loadAudit()]);renderSummary()}catch(error){notice(error.message,'error')}
}
$('#submit-review').onclick=async()=>{
 if(!state.selected)return;
 try{await request('/partner/restaurants/'+activeId()+'/submit',{method:'POST',body:JSON.stringify({version:state.selected.version}),headers:{'Idempotency-Key':crypto.randomUUID()}});notice('Submitted for independent review. It is still hidden from customers.','success');await refreshSelected()}catch(error){notice(error.message,'error')}
};

showAuth(true);setAuthMode('login');authenticate();
