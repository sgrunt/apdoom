let fs = require('fs');

let regions = JSON.parse(fs.readFileSync("regions.json").toString());

let doom = {maps:[]}
let doom2 = {maps:[]}

regions.maps.forEach(map => {
    if (map.d2_map == -1)
    {
        doom.maps.push(map);
    }
    else
    {
        map.ep = 0;
        map.map = map.d2_map;
        doom2.maps.push(map);
    }
});

fs.writeFileSync("DOOM 1993.json", JSON.stringify(doom))
fs.writeFileSync("DOOM II.json", JSON.stringify(doom2))
