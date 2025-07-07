# mostly from https://commons.wikimedia.org/wiki/User:Inductiveload/gallery#Tue_Jul_14_22:44:20_BST_2009
wget -nc https://upload.wikimedia.org/wikipedia/commons/3/36/Screw_Head_-_Hex_External_with_round_base.svg -O head_hex.svg
wget -nc https://upload.wikimedia.org/wikipedia/commons/f/f3/Screw_Head_-_Slotted.svg -O head_flat.svg
wget -nc https://upload.wikimedia.org/wikipedia/commons/3/3f/Screw_Head_-_Phillips.svg -O head_phillips.svg
wget -nc https://upload.wikimedia.org/wikipedia/commons/b/ba/Screw_Head_-_Hex_External.svg -O hex_external.svg

# derived from hex_external.svg
echo '<svg viewBox="0 0 40 40" xmlns="http://www.w3.org/2000/svg"><path d="M38.5 20 29 36H11L1.5 20 11 4H29Z" fill="none" stroke="#000" stroke-width="2"/><circle r="10" cx="20" cy="20" fill="none" stroke="#000" stroke-width="2"/></svg>
' >> nut_hex.svg