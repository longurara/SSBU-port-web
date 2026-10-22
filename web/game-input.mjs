// Npad button/stick layout and keyboard bindings match runtime/nnhid.c.
const keys=new Map([['KeyZ',0],['KeyX',1],['KeyC',2],['KeyV',3],['KeyQ',6],['KeyE',7],['KeyA',8],['KeyS',9],['Enter',10],['Backspace',11],['KeyJ',12],['KeyI',13],['KeyL',14],['KeyK',15]]);
const arrows=new Set(['ArrowLeft','ArrowUp','ArrowRight','ArrowDown']);
const padButtons=[1,0,3,2,6,7,8,9,11,10,4,5,13,15,12,14];
const axis=n=>{if(!Number.isFinite(n))return 0;const v=Math.max(-1,Math.min(1,n)),a=Math.abs(v);return a<=0.15?0:Math.round(Math.sign(v)*(a-0.15)/0.85*32767);};
const stronger=(a,b)=>Math.abs(a)>=Math.abs(b)?a:b;
export function padState(held=new Set(),gamepad=null) {
  let buttons=0,lx=0,ly=0,rx=0,ry=0;
  for(const code of held)if(keys.has(code))buttons|=1<<keys.get(code);
  if(gamepad?.connected&&gamepad.mapping==='standard') {
    for(let i=0;i<padButtons.length;i++)if(gamepad.buttons[i]?.pressed||gamepad.buttons[i]?.value>0.5)buttons|=1<<padButtons[i];
    [lx,ly,rx,ry]=[axis(gamepad.axes[0]),-axis(gamepad.axes[1]),axis(gamepad.axes[2]),-axis(gamepad.axes[3])];
  }
  lx=stronger((Number(held.has('ArrowRight'))-Number(held.has('ArrowLeft')))*32767,lx);
  ly=stronger((Number(held.has('ArrowUp'))-Number(held.has('ArrowDown')))*32767,ly);
  for(const [x,y,shift] of [[lx,ly,16],[rx,ry,20]]){if(x< -16383)buttons|=1<<shift;if(y>16383)buttons|=1<<(shift+1);if(x>16383)buttons|=1<<(shift+2);if(y< -16383)buttons|=1<<(shift+3);}
  return [buttons>>>0,0,lx|0,ly|0,rx|0,ry|0];
}
export function writePadState(words,state) {
  if(!(words instanceof Int32Array)||!(words.buffer instanceof SharedArrayBuffer)||words.length!==7||!Array.isArray(state)||state.length!==6)throw new Error('Invalid shared Npad state');
  const sequence=(Atomics.load(words,0)+1)|1;Atomics.store(words,0,sequence);
  for(let i=0;i<6;i++)Atomics.store(words,i+1,state[i]|0);
  Atomics.store(words,0,(sequence+1)|0);
}
export function attachGameInput(canvas) {
  const held=new Set();let words=null,closed=false,frame;
  canvas.tabIndex=0;
  const sample=()=>{if(!words)return;const active=document.activeElement===canvas&&document.hasFocus()&&!document.hidden;
    const gamepad=active?Array.from(navigator.getGamepads?.()||[]).find(p=>p?.connected&&p.mapping==='standard'):null;
    writePadState(words,active?padState(held,gamepad):padState());};
  const key=(event,down)=>{if(!keys.has(event.code)&&!arrows.has(event.code))return;
    if(down&&(event.ctrlKey||event.metaKey||event.altKey))return;
    event.preventDefault();if(down)held.add(event.code);else held.delete(event.code);sample();};
  const down=e=>key(e,true),up=e=>key(e,false),reset=()=>{held.clear();if(words)writePadState(words,padState());};
  const poll=()=>{if(closed)return;sample();frame=requestAnimationFrame(poll);};
  canvas.addEventListener('keydown',down);canvas.addEventListener('keyup',up);canvas.addEventListener('blur',reset);
  window.addEventListener('blur',reset);document.addEventListener('visibilitychange',reset);
  canvas.addEventListener('pointerdown',focus);
  function focus(){canvas.focus();}
  poll();focus();
  return {
    bind(buffer,offset){if(closed)return;if(!(buffer instanceof SharedArrayBuffer)||!Number.isInteger(offset)||offset<0||offset%4||offset+28>buffer.byteLength)throw new Error('Invalid Npad shared pointer');words=new Int32Array(buffer,offset,7);sample();},
    close(){if(closed)return;reset();closed=true;cancelAnimationFrame(frame);canvas.removeEventListener('keydown',down);canvas.removeEventListener('keyup',up);canvas.removeEventListener('blur',reset);canvas.removeEventListener('pointerdown',focus);window.removeEventListener('blur',reset);document.removeEventListener('visibilitychange',reset);words=null;}
  };
}
