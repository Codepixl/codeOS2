const char *fat32sig = "FAT32   ";
fat32part currentfat32part;
bool isPartitionFAT32(int disk, int sect){
	uint8_t buf[512];
	readSector(disk, sect, buf);
	bool flag = true;
	for(int i = 0; i < 8; i++){
		if(buf[0x52+i] != fat32sig[i])
			flag = false;
	}
	return flag;
}

fat32part getFat32Part(int disk, int part_sect){
	fat32part temp;
	temp.disk = disk;
	temp.part_sect = part_sect;
	uint8_t sect[512];
	readSector(disk, part_sect, sect);
	for(int i = 0; i < 512; i++){
		if(i == 0xD){
			temp.sectors_per_cluster = sect[i];
		}
		if(i >= 0xE && i <= 0xF){
			if(i == 0x0E){
				temp.reserved_sectors = 0;
			}
			temp.reserved_sectors += ((uint16_t)sect[i]) << (i-0xE)*8;
		}
		if(i == 0x10){
			temp.num_fats = sect[i];
		}
		if(i >= 0x24 && i <=0x28){
			if(i == 0x24){
				temp.sectors_per_fat = 0;
			}
			if(i >= 0x24 && i <= 0x25){
				temp.sectors_per_fat += ((uint32_t)sect[i]) << (i-0x24)*8;
			}else{
				temp.sectors_per_fat += ((uint32_t)sect[i]) << (i-0x27)*8+16;
			}
		}
		if(i >= 0x2C && i <= 0x2F){
			if(i == 0x2C){
				temp.root_dir_clust = 0;
			}
			if(i >= 0x2C && i <= 0x2D){
				temp.root_dir_clust += ((uint32_t)sect[i]) << (i-0x2C)*8;
			}else{
				temp.root_dir_clust += ((uint32_t)sect[i]) << (i-0x2E)*8+16;
			}
		}
	}
	temp.cluster_begin_sect = part_sect + temp.reserved_sectors + (temp.num_fats * temp.sectors_per_fat);
	temp.root_dir_sect = clusterToLBAOther(temp, temp.root_dir_clust);
	return temp;
}

void listDir(uint32_t cluster){
	bool done = false;
	uint32_t sector = clusterToLBA(cluster);
	uint8_t sect[512];
	while(!done){
		readSector(currentfat32part.disk, sector, sect);
		for(uint8_t i = 0; i < 16; i++){
			uint16_t loc = 0x20*i;
			if(sect[loc] == 0){ //End of directory
				done = true;
				i = 16;
			}else if(sect[loc] != 0xE5 /*Is not unused*/ 
					&& (sect[loc+0xB] & 0xF) != 0xF /*Is not a long filename entry*/ 
					&& (sect[loc+0xB] & 0xA) == 0 /*Is a file or directory and should be shown*/){
				uint8_t lastChar = 0;
				for(int i = 0; i < 8; i++){
					if(sect[loc+i] != ' '){
						lastChar = i;
					}
				}
				if(sect[loc+0xB] & 0x10){ //Is a directory
					char name[lastChar+2];
					memcpy(&name, &sect[loc], lastChar+1);
					name[lastChar+1] = 0;
					print(name);
					for(uint8_t j = 0; j <= 13-(lastChar+1); j++){
						print(" ");
					}
					println("<DIR>");
				}else{
					char name[lastChar+6];
					memcpy(&name, &sect[loc], lastChar+1);
					memcpy(&name[lastChar+1], &sect[loc+7], 4);
					name[lastChar+5] = 0;
					name[lastChar+1] = '.';
					println(name);
				}
			}else{
				
			}
		}
		sector++;
	}
}

uint32_t getClusterOfEntry(uint8_t *entry){
	return (((uint32_t)(entry[0x1A])))+(((uint32_t)(entry[0x1B])) << 8)+(((uint32_t)(entry[0x14])) << 16)+(((uint32_t)(entry[0x15])) << 24);
}

void setCurrentFat32part(fat32part p){
	currentfat32part = p;
}

uint32_t clusterToLBA(uint32_t cluster){
	return currentfat32part.cluster_begin_sect+(currentfat32part.sectors_per_cluster * (cluster-2));
}

uint32_t clusterToLBAOther(fat32part p, uint32_t cluster){
	return p.cluster_begin_sect+(p.sectors_per_cluster * (cluster-2));
}