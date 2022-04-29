//Made by Alexander Bagherzadeh
//EEL 4678 Computer Architecture
//Project 1 Cache Simulation

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

//stores the attributes of the cache
struct cacheData {
    unsigned int cacheSize;
    unsigned int indexSize;
    unsigned int numOfSets;
    unsigned int replacementPolicy;
    unsigned int writePolicy;
    unsigned int blockOffset;
    char * filePath;
};

//stores the attributes to be stored in the full cache array as well as the current line that is being configured
struct lineData {
    char dirtyBit;
    unsigned long long address;
};

//stores values from the input line of the trace file to be configured as well as the calculated index and tag
struct addressData {
    char readOrWrite;
    char address[64];
    unsigned long long currentIndex;
    unsigned long long currentTag;
};

//stores information about masks for the tag/index/blockoffset for this specific cache configuration as well as the amount of bits of each for future calculations
struct maskData {
    int blockOffsetMask;
    int indexMask;
    unsigned long long tagMask;
    int blockOffsetBits;
    int indexBits;
    int tagBits;
};

//stores the counting data to be represented later when the program finishes
struct hitData {
    unsigned long long hits;
    unsigned long long misses;
    unsigned long long reads;
    unsigned long long writes;
};

//funcion prototypes
void getIndex(struct cacheData *cache);
void declareCache(struct lineData **fullCache, struct cacheData *cache);
void declareReplacementPolicyStoring(unsigned long long **replacementPolicyStoring, struct cacheData *cache);
void flushCache(struct lineData **fullCache, struct cacheData *cache);
void flushReplacementPolicy(unsigned long long **replacementPolicyStoring, struct cacheData *cache);
void printCache(struct lineData **fullCache, struct cacheData *cache);
void printReplacementPolicyStoring(unsigned long long **replacementPolicyStoring, struct cacheData *cache);
void getMasks(struct maskData *mask, struct cacheData *cache);
void getCurrentIndexAndTag(struct maskData *mask, struct addressData *address, struct lineData *currentLine, struct cacheData *cache);

int main(int argc, char *argv[])
{
    //initialization of variables
    FILE *cfPtr;
    struct cacheData cache = {0, 0, 0, 0, 0, 0, ""};
    struct maskData mask;
    struct hitData hitMiss;
    hitMiss.hits = 0;
    hitMiss.misses = 0;
    hitMiss.reads = 0;
    hitMiss.writes = 0;

    //allocate space for file path
    cache.filePath = (char *)malloc(sizeof(argv[5])*sizeof(char));

    //store arguments
    cache.cacheSize = atoi(argv[1]);
    cache.numOfSets = atoi(argv[2]);
    cache.replacementPolicy = atoi(argv[3]);
    cache.writePolicy = atoi(argv[4]);
    strcpy(cache.filePath,argv[5]);
    cache.blockOffset = 64;
    //calculate index
    getIndex(&cache);

    //generate masks for later use in finding the current tag and current index
    getMasks(&mask,&cache);
    
    //Allocates space for the full cache
    struct lineData **fullCache = (struct lineData **)malloc((int)cache.numOfSets * sizeof(struct lineData *));

    for (int i = 0; i < cache.numOfSets; ++i)
    {
        fullCache[i] = (struct lineData *)malloc((int)cache.indexSize * sizeof(struct lineData));
    }
    
    //A full cache copy to store replacement policy order
    unsigned long long **replacementPolicyStoring = (unsigned long long **)malloc((int)cache.numOfSets * sizeof(unsigned long long *));

    for (int i = 0; i < cache.numOfSets; ++i)
    {
        replacementPolicyStoring[i] = (unsigned long long *)malloc((int)cache.indexSize * sizeof(unsigned long long));
    }

    //initial declaration of the full cache
    declareCache(fullCache, &cache);

    //initial declaration of the replacement policy storing
    declareReplacementPolicyStoring(replacementPolicyStoring, &cache);
    
    if ((cfPtr = fopen(cache.filePath, "r")) == NULL) { //file declaration
		puts("File could not be opened.");
	}
	else {
        //initialization of variables
        struct addressData address;
        struct lineData currentLine;
        char lineString[64];
        bool insideCache = false;
        bool storedInCache = false;
        int replacementLocation;
        //while loop goes until end of file
        while(fgets(lineString,64,cfPtr) != NULL) {
        sscanf(lineString,"%c %s",&address.readOrWrite,&address.address);
            currentLine.address = strtoll(address.address, NULL, 16);

            //Sets the dirty bit if its write back as well as counts the writes for write through
            if ((address.readOrWrite == 'W' || address.readOrWrite == 'w') && cache.writePolicy == 1) {
                currentLine.dirtyBit = 'D';
            } else if (address.readOrWrite == 'W' || address.readOrWrite == 'w') {
                currentLine.dirtyBit = ' ';
                hitMiss.writes++;
            } else {
                currentLine.dirtyBit = ' ';
            }

            //gets current index and tags by using previously generated masks
            getCurrentIndexAndTag(&mask,&address,&currentLine,&cache);
            
            //checks if current tag is inside the cache
            for (int i = 0; i < (cache.numOfSets); i++) {
                if (fullCache[i][address.currentIndex].address == address.currentTag) {
                    insideCache = true;
                    hitMiss.hits++;
                    if (currentLine.dirtyBit == 'D') {
                        fullCache[i][address.currentIndex].dirtyBit = currentLine.dirtyBit;
                    }
                    break;
                } else {
                    insideCache = false;
                }
            }
            //increment misses and reads if not in cache and store it in cache
            if (!insideCache) {
                hitMiss.misses++;
                hitMiss.reads++;
                for (int i = 0; i < (cache.numOfSets); i++) {
                    if (fullCache[i][address.currentIndex].address == 0) {
                        fullCache[i][address.currentIndex].address = address.currentTag; 
                        fullCache[i][address.currentIndex].dirtyBit = currentLine.dirtyBit; 
                        storedInCache = true;
                        break;
                    }
                }
                //if there is no empty space, replace lru or fifo specified location and count writes if evicting a tag that is marked dirty
                if (!storedInCache) {
                    for (int i = 0; i < (cache.numOfSets); i++) {
                        if (fullCache[i][address.currentIndex].address == replacementPolicyStoring[cache.numOfSets - 1][address.currentIndex]) {
                            //implements dirty eviction check
                            if ((fullCache[i][address.currentIndex].dirtyBit == 'D') && (fullCache[i][address.currentIndex].address != address.currentTag))  {
                                hitMiss.writes++;
                            } 
                            fullCache[i][address.currentIndex].address = address.currentTag;
                            fullCache[i][address.currentIndex].dirtyBit = currentLine.dirtyBit;  
                            storedInCache = true;
                        }
                    }
                }
                storedInCache = false;
            }
            //switch for replacement policy implementation
            switch (cache.replacementPolicy) {
                case 0:
                    //Case for LRU Storing
                    if (insideCache) {
                        for (int i = 0; i < (cache.numOfSets); i++) {
                            if(replacementPolicyStoring[i][address.currentIndex] == address.currentTag) {
                                replacementLocation = i; 
                                break;
                            }
                        }
                        for (int j = replacementLocation; j >= 0; j--) {
                            if (j == 0) {
                                replacementPolicyStoring[j][address.currentIndex] = address.currentTag;
                                break;
                            }
                            replacementPolicyStoring[j][address.currentIndex] = replacementPolicyStoring[j - 1][address.currentIndex];
                        }
                    } else {
                        for (int j = (cache.numOfSets - 1); j >= 0; j--) {
                            if (j == 0) {
                                replacementPolicyStoring[j][address.currentIndex] = address.currentTag;
                                break;
                            } else {
                                replacementPolicyStoring[j][address.currentIndex] = replacementPolicyStoring[j - 1][address.currentIndex];
                            }
                        }
                    }
                    break;
                case 1:
                   // case for FIFO storing
                   if (!insideCache) {
                        for (int j = (cache.numOfSets - 1); j >= 0; j--) {
                            if (j == 0) {
                                replacementPolicyStoring[j][address.currentIndex] = address.currentTag;
                            } else {
                                replacementPolicyStoring[j][address.currentIndex] = replacementPolicyStoring[j - 1][address.currentIndex];
                            }
                        }
                    }
                    break;
                default:
                break;
            }
        }
		fclose(cfPtr);
	}
    unsigned long long totalHitsAndMisses = hitMiss.hits + hitMiss.misses;

    double missRatio = hitMiss.misses / (totalHitsAndMisses * 1.0);
    printf("%f\n%lld\n%lld\n", missRatio, hitMiss.writes, hitMiss.reads);

    //can be used to print out the tags in the cache and replacement policy storing to check/verify functionality
    //printCache(fullCache, &cache);
    //printReplacementPolicyStoring(replacementPolicyStoring, &cache);

    flushReplacementPolicy(replacementPolicyStoring, &cache);

    flushCache(fullCache, &cache);
    
    free(cache.filePath);

    return 0;
}

//function for index calculation
void getIndex(struct cacheData *cache) {
    cache->indexSize = cache->cacheSize / ((cache->blockOffset) * (cache->numOfSets));
}

//funciton for initialization of fullCache tags
void declareCache(struct lineData **fullCache, struct cacheData *cache) {
    for (int i = 0; i < (cache->indexSize); i++)
    {
        for (int j = 0; j < (cache->numOfSets); j++)
        {
            (fullCache[j][i].address) = 0;
        }
    }
}

//function to print tags of the fullCache
void printCache(struct lineData **fullCache, struct cacheData *cache) {
    for (int i = 0; i < (cache->indexSize); i++) 
    {
        for (int j = 0; j < (cache->numOfSets); j++)
        {
            printf("%lld ", (fullCache[j][i].address));
        }
        printf("\n");
    }
    printf("\n");
    printf("\n");
}

//funciton to initialize replacementPolicyStoring
void declareReplacementPolicyStoring(unsigned long long **replacementPolicyStoring, struct cacheData *cache) {
    for (int i = 0; i < (cache->indexSize); i++)
    {
        
        for (int j = 0; j < (cache->numOfSets); j++)
        {
            
            replacementPolicyStoring[j][i] = 0;
        }
        
    }
}  

//function to print full replacementPolicyStoring
void printReplacementPolicyStoring(unsigned long long **replacementPolicyStoring, struct cacheData *cache) {
    for (int i = 0; i < (cache->indexSize); i++)
    {
        for (int j = 0; j < (cache->numOfSets); j++)
        {
            printf("%lld ", replacementPolicyStoring[j][i]);
        }
        printf("\n");
    }
    printf("\n");
    printf("\n");
}

//free memory for fullCache
void flushCache(struct lineData **fullCache, struct cacheData *cache) {
    for (int i = 0; i < cache->numOfSets; i++)
    {
        free(fullCache[i]);
    }
    free(fullCache);
}

//free memory for replacementPolicyStoring
void flushReplacementPolicy(unsigned long long **replacementPolicyStoring, struct cacheData *cache) {
    for (int i = 0; i < cache->numOfSets; i++)
    {
        free(replacementPolicyStoring[i]);
    }
    free(replacementPolicyStoring);
}

//function for mask generation
void getMasks(struct maskData *mask, struct cacheData *cache) {
    mask->blockOffsetMask = (cache->blockOffset) - 1;
    mask->blockOffsetBits = log2((mask->blockOffsetMask) + 1);
    mask->indexBits = log2(cache->indexSize);
    mask->indexMask = ((cache->indexSize) - 1) << (mask->blockOffsetBits);
    mask->tagBits = 64 - ((mask->blockOffsetBits) + (mask->indexBits));
    mask->tagMask = ((int)pow(2, (mask->tagBits)) - 1) << ((mask->blockOffsetBits) + (mask->indexBits));
}

//function for current index and tag
void getCurrentIndexAndTag(struct maskData *mask, struct addressData *address, struct lineData *currentLine, struct cacheData *cache) {
    address->currentIndex = ((mask->indexMask) & (currentLine->address)) >> mask->blockOffsetBits;
    address->currentTag = ((mask->tagMask) & (currentLine->address)) >> ((mask->blockOffsetBits) + (mask->indexBits));
}
