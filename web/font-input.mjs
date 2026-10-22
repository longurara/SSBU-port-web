export const requiredFonts=Object.freeze([
  'nintendo_udsg-r_std_003.ttf','nintendo_udsg-r_org_zh-cn_003.ttf',
  'nintendo_udsg-r_ext_zh-cn_003.ttf','nintendo_udjxh-db_zh-tw_003.ttf',
  'nintendo_udsg-r_ko_003.ttf','nintendo_ext_003.ttf','nintendo_ext2_003.ttf',
]);

// WORKERFS receives local File objects. No file bytes are sent to the server.
export function localFontInputs(files,{required=false}={}) {
  const chosen=new Map();
  for(const file of files??[]) {
    if(!(file instanceof File))throw new Error('Dữ liệu font cục bộ không hợp lệ.');
    if(!requiredFonts.includes(file.name))continue;
    if(chosen.has(file.name))throw new Error(`Trùng file font: ${file.name}`);
    if(!file.size||file.size>32*1024*1024)throw new Error(`Kích thước font không hợp lệ: ${file.name}`);
    chosen.set(file.name,file);
  }
  const missing=requiredFonts.filter(name=>!chosen.has(name));
  if(required&&missing.length)throw new Error(`Chọn đủ 7 font hệ thống trong thư mục fonts. Còn thiếu: ${missing.join(', ')}`);
  return requiredFonts.filter(name=>chosen.has(name)).map(name=>({name,data:chosen.get(name)}));
}
