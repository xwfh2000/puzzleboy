#include "TestSolver.h"
#include "MyFormat.h"
#include "PooledAllocator.h"

#define USE_CUSTOM_HASHTREE

//unfortunately, it breaks in Android (probably all ARM architecture)
#if defined(ANDROID) || defined(__IPHONEOS__)
#undef USE_CUSTOM_HASHTREE
#endif

#ifdef USE_CUSTOM_HASHTREE
#include "SimpleHashAVLTree.h"
#include "SimpleHashRedBlackTree.h"
#else
#include <map>
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <vector>

//#define SOLVER_PROFILING

#ifdef SOLVER_PROFILING
#include <SDL.h>
#endif

//test solver for level with rotate blocks only

struct TestRotateBlock{
	unsigned char pos;
	unsigned char type;
	//type=0x0 1000 0100 0010 0001
	//type=0x4 1100 0110 0011 1001
	//type=0x8 1010 0101
	//type=0xC 1110 0111 1011 1101
	//type=0xA 1111
	unsigned char shift;
	unsigned char mask; //0(type=0xA) or 1(type=0x8) or 3(else)
};

typedef unsigned long long TestSolverStateType;

struct TestSolverNode{
	TestSolverNode* parent;
	TestSolverNode* next; //the queue
	TestSolverStateType state;

	unsigned int HashValue() const{
		return (unsigned int)state;
	}

	char Compare(const TestSolverNode& other) const{
		return (state<other.state)?-1:((state>other.state)?1:0);
	}
};

static const unsigned char HitTestLUT[16]={
	1,2,4,8,
	3,6,12,9,
	5,10,15,0,
	7,14,13,11,
};

static const char dirLUT[4]={-16,-1,16,1};

//return value: bit0-5: index, bit6-7: direction, bit8-11: current type
//-1=none
static int TestSolver_HitTestForBlocks(const unsigned char mapAdjacency[256][4],const TestRotateBlock* blocks,TestSolverStateType state,unsigned char pos){
	for(int i=0;i<4;i++){
		unsigned char c=mapAdjacency[pos][i];
		if(c==0xFF) break;

		int idx=c&0x3F;
		int ii=blocks[idx].type;
		if(blocks[idx].mask) ii+=(state>>blocks[idx].shift) & blocks[idx].mask;
		if(HitTestLUT[ii] & (1<<(c>>6))) return (ii<<8)|c;
	}

	return -1;
}

static bool TestSolver_CalculateAdjacancyData(unsigned char mapAdjacency[256][4],const unsigned char* mapData,const TestRotateBlock* blocks,int blockCount){
	memset(mapAdjacency,-1,1024);

	unsigned char counts[256]={};

	for(int ii=0;ii<blockCount;ii++){
		for(int d=0;d<4;d++){
			unsigned char idx=blocks[ii].pos+dirLUT[d];
			unsigned char count=counts[idx];
			if(mapData[idx]&0x2){ //if block is movable
				if(count>=4){
					printf("[TestSolver] Error: Too many adjacent blocks\n");
					return false;
				}
				mapAdjacency[idx][count]=ii|(d<<6);
				counts[idx]++;
			}
		}
	}

	return true;
}

static TestSolverStateType TestSolver_SortPosition(int count,unsigned char size,unsigned char offset,TestSolverStateType pos){
	switch(count){
	case 2:
		{
			unsigned char mask=(1<<size)-1;
			unsigned char p1=(pos>>offset)&mask;
			unsigned char p2=(pos>>(offset+size))&mask;

			if(p1>p2) return (TestSolverStateType(p2)<<offset)
				|(TestSolverStateType(p1)<<(offset+size))
				|(pos&~(((TestSolverStateType(1)<<(size*2))-1)<<offset));
		}
		break;
	case 3:
	case 4:
		{
			unsigned char mask=(1<<size)-1;
			unsigned char p1=(pos>>offset)&mask;
			unsigned char p2=(pos>>(offset+size))&mask;
			unsigned char p3=(pos>>(offset+size*2))&mask;
			unsigned char p4=(pos>>(offset+size*3))&mask;

			if(count==4){
				if(p1>p2){
					if(p1>p3){
						if(p1>p4){
							std::swap(p1,p4); //p1 biggest, swap to p4
						}else{ //p1<=p4
							std::swap(p1,p3); //p1 second biggest, swap to p3
							if(p1>p2) std::swap(p1,p2);
							count=0; //all numbers are sorted
						}
					}else{ //p2<p1<=p3
						if(p1>p4){
							unsigned char tmp=p4;
							p4=p3; //p3 biggest, swap to p4
							p3=p1; //p1 second biggest, swap to p3
							if(p2>tmp) p1=tmp; //tmp smallest
							else p1=p2,p2=tmp; //p2 smallest, swap to p1
						}else{
							std::swap(p1,p2); //p1 second smallest, p2 smallest, swap
							if(p3>p4) std::swap(p3,p4);
						}
						count=0; //all numbers are sorted
					}
				}else{ //p1<=p2
					if(p2>p3){
						if(p2>p4){
							std::swap(p2,p4); //p2 biggest, swap to p4
						}else{ //p1,p3<=p2<=p4
							std::swap(p2,p3); //p2 second biggest, swap to p3
							if(p1>p2) std::swap(p1,p2);
							count=0; //all numbers are sorted
						}
					}else{ //p1<=p2<=p3
						if(p2>p4){
							unsigned char tmp=p4;
							p4=p3; //p3 biggest, swap to p4
							p3=p2; //p2 second biggest, swap to p3
							if(p1>tmp) p2=p1,p1=tmp; //tmp smallest, swap to p1
							else p2=tmp; //p2 smallest
						}else{ //p2<=p4
							if(p3>p4){
								std::swap(p3,p4);
							}else{
								break; //order is already correct
							}
						}
						count=0; //all numbers are sorted
					}
				}
			}

			if(count){
				if(p1>p2){
					if(p1>p3){
						std::swap(p1,p3); //p1 biggest, swap to p3
						if(p1>p2) std::swap(p1,p2);
					}else{ //p1<=p3
						std::swap(p1,p2);
					}
				}else{ //p1<=p2
					if(p2>p3){
						std::swap(p2,p3); //p2 biggest, swap to p3
						if(p1>p2) std::swap(p1,p2);
					}else{ //p2<=p3
						if(count==3) break; //order is already correct
					}
				}
			}

			if(count==3){
				assert(p1<=p2 && p2<=p3);
				return (TestSolverStateType(p1)<<offset)
					|(TestSolverStateType(p2)<<(offset+size))
					|(TestSolverStateType(p3)<<(offset+size*2))
					|(pos&~(((TestSolverStateType(1)<<(size*3))-1)<<offset));
			}else{
				assert(p1<=p2 && p2<=p3 && p3<=p4);
				return (TestSolverStateType(p1)<<offset)
					|(TestSolverStateType(p2)<<(offset+size))
					|(TestSolverStateType(p3)<<(offset+size*2))
					|(TestSolverStateType(p4)<<(offset+size*3))
					|(pos&~(((TestSolverStateType(1)<<(size*4))-1)<<offset));
			}
		}
		break;
	default:
		break;
	}

	return pos;
}

struct PlayerPosition{
	unsigned char& operator[](int index){
		return p[index];
	}
	unsigned char p[4];
};

int TestSolver_SolveIt(const PuzzleBoyLevel& level,u8string* rec,void* userData,LevelSolverCallback callback,TestSolverExtendedData *ed){
#ifdef SOLVER_PROFILING
	int ttt=SDL_GetTicks();
#endif

	//type|(dir<<4)
	const unsigned char LUT1[16]={
		0xFF,0x00,0x10,0x04, //0000 1000 0100 1100
		0x20,0x08,0x14,0x0C, //0010 1010 0110 1110
		0x30,0x34,0x18,0x3C, //0001 1001 0101 1101
		0x24,0x2C,0x1C,0x0A, //0011 1011 0111 1111
	};

	//[type][isCW]
	//the value is 0=end
	//567
	//9 B
	//DEF
	const unsigned short LUT2[16][2]={
		{0x0059,0x007B},{0x00DE,0x0056},{0x00FB,0x00D9},{0x0076,0x00FE},
		{0x05DE,0x057B},{0x0DFB,0x0D56},{0x0F76,0x0FD9},{0x0759,0x07FE},
		{0x59FB,0x7BD9},{0xDE76,0x56FE},{0x57DF,0x57DF},{0x0000,0x0000},
		{0x5DFB,0xD57B},{0xDF76,0xFD56},{0xF759,0x7FD9},{0x75DE,0x57FE},
	};

	if(ed) memset(ed,0,sizeof(TestSolverExtendedData));

	//================ init solver
	if(level.m_nWidth>15 || level.m_nHeight>15){
		printf("[TestSolver] Error: Map too big\n");
		return -2;
	}

	//currently up to 4 players are supported (??? experimental)
	if(level.m_nPlayerCount<1 || level.m_nPlayerCount>4){
		printf("[TestSolver] Error: Invalid player count\n");
		return -2;
	}

	const int width=level.m_nWidth;
	const int height=level.m_nHeight;

	const unsigned char playerXSize=(width<=4)?((width<=2)?1:2):((width<=8)?3:4);
	const unsigned char playerYSize=(height<=4)?((height<=2)?1:2):((height<=8)?3:4);
	unsigned char stateSize=0;
	unsigned char exitPos=0; //position of one of exit (y+1)*16+(x+1)

	//currently 0-4 boxes are supported (??? experimental)
	//FIXME: completely wrong if there are at least 2 normal blocks and one is filling to the hole
	//because filling order is not saved
	//FIXME: currently only rectangular boxes are supported
	unsigned char boxWidth[4]={},boxHeight[4]={},boxXShift[4]={},boxYShift[4]={},boxXMask[4]={},boxYMask[4]={};
	unsigned char boxCount=0,boxTarget=0,useDeadlock=0;
	unsigned char boxStateSize=0; //>0 means all boxes are of same type, and this is the state size

	TestSolverStateType initState=0;

	//load map, format:
	//bit0=player is movable
	//bit1=block is movable
	//bit2=is exit
	//bit3-5=block occupied (and block index) (??? experimental !!!)
	unsigned char mapData[256]={};

	for(int j=0;j<level.m_nHeight;j++){
		for(int i=0;i<level.m_nWidth;i++){
			unsigned char c=level(i,j);
			switch(c){
			case FLOOR_TILE:
				c=0x3;
				break;
			case WALL_TILE:
				c=0x0;
				break;
			case EMPTY_TILE:
				c=0x2;
				break;
			case EXIT_TILE:
				c=0x7; //c=0x5; //< ???? this is correct behavior but it breaks old test file and random map generator (???)
				exitPos=(j+1)*16+(i+1);
				break;
			case PLAYER_TILE:
				c=0x3;
				initState|=TestSolverStateType(i|(j<<playerXSize))<<stateSize;
				stateSize+=playerXSize+playerYSize;
				break;
			default:
				printf("[TestSolver] Error: Invalid block type\n");
				return -2;
			}
			mapData[(unsigned char)((j+1)*16+(i+1))]=c;
		}
	}

	//(experimental) pushable box support
	for(size_t i=0;i<level.m_objBlocks.size();i++){
		const PushableBlock &obj=*level.m_objBlocks[i];

		if(obj.m_nType==ROTATE_BLOCK) continue;

		if(obj.IsSolid()){
			if(boxCount<4){
				boxWidth[boxCount]=obj.m_w;
				boxHeight[boxCount]=obj.m_h;
				if(obj.m_nType==TARGET_BLOCK){
					boxTarget|=(1<<boxCount);
					if(obj.m_w==1 && obj.m_h==1) useDeadlock|=(1<<boxCount);
				}

				if(obj.m_w==0 || obj.m_h==0 || obj.m_w>=width || obj.m_h>=height){
					printf("[TestSolver] Error: Invalid block size\n");
					return -2;
				}

				int w=width+1-obj.m_w,h=height+1-obj.m_h;
				const unsigned char boxXSize=(w<=4)?((w<=2)?1:2):((w<=8)?3:4);
				const unsigned char boxYSize=(h<=4)?((h<=2)?1:2):((h<=8)?3:4);
				boxXShift[boxCount]=stateSize;
				boxYShift[boxCount]=stateSize+boxXSize;
				stateSize+=boxStateSize=boxXSize+boxYSize;
				boxXMask[boxCount]=(1<<boxXSize)-1;
				boxYMask[boxCount]=(1<<boxYSize)-1;

				initState|=(TestSolverStateType(obj.m_x&boxXMask[boxCount])<<boxXShift[boxCount])
					|(TestSolverStateType(obj.m_y&boxYMask[boxCount])<<boxYShift[boxCount]);

				boxCount++;

				continue;
			}else{
				printf("[TestSolver] Error: Too many blocks\n");
				return -2;
			}
		}

		printf("[TestSolver] Error: Invalid block type\n");
		return -2;
	}

	//check if all boxes are of same size
	if(boxCount>=2){
		for(int boxIndex=1;boxIndex<boxCount;boxIndex++){
			if(boxWidth[boxIndex]!=boxWidth[0] ||
				boxHeight[boxIndex]!=boxHeight[0] ||
				((boxTarget^(boxTarget>>boxIndex))&1))
			{
				boxStateSize=0;
				break;
			}
		}
	}else{
		boxStateSize=0;
	}

	//for deadlock optimization later
	unsigned char newSize=stateSize;

	//add rotate blocks
	TestRotateBlock blocks[TestSolver_MaxBlockCount];
	int blockCount=0;
	for(size_t i=0;i<level.m_objBlocks.size();i++){
		const PushableBlock &obj=*level.m_objBlocks[i];

		if(obj.m_nType!=ROTATE_BLOCK) continue;

		if(blockCount>TestSolver_MaxBlockCount){
			printf("[TestSolver] Error: Too many blocks\n");
			return -2;
		}

		TestRotateBlock block={(obj.m_y+1)*16+(obj.m_x+1),0,0,0};
		unsigned char dir=0;
		for(int j=0;j<4;j++){
			unsigned char c=obj[j];
			if(c>1){
				printf("[TestSolver] Error: Invalid block size\n");
				return -2;
			}
			block.type|=(c<<j);
		}

		block.type=LUT1[block.type&0xF];
		if(block.type==0xFF){
			printf("[TestSolver] Error: Invalid block size\n");
			return -2;
		}

		dir=block.type>>4;
		block.type&=0xF;

		//calculate state bits
		if(block.type!=0xA){
			unsigned char blockStateSize=(block.type==0x8)?1:2;
			block.shift=stateSize;
			stateSize+=blockStateSize;
			if(stateSize>sizeof(TestSolverStateType)*8){
				printf("[TestSolver] Error: Too many blocks\n");
				return -2;
			}
			block.mask=(1<<blockStateSize)-1;
			initState|=((TestSolverStateType)dir)<<block.shift;
		}

		//add block
		mapData[block.pos]&=~0x3;
		blocks[blockCount]=block;
		blockCount++;
	}

	//ad-hoc: check additional exit point
	unsigned char exitCount=level.m_nExitCount;
	{
		u8string s=toUTF8(level.m_sLevelName);
		u8string::size_type lps=s.find("(E=");
		if(lps!=u8string::npos){
			int x,y;
			if(sscanf(&(s[lps+3]),"%d,%d",&x,&y)==2){
				unsigned char p=y*16+x;
				if(x>=1 && x<=width && y>=1 && y<=height){
					exitPos=p;
					mapData[p]|=0x4;
					exitCount++;
				}
			}
		}
		if(exitCount<=0 && !boxTarget){
			printf("[TestSolver] Error: Invalid exit count\n");
			return -2;
		}
	}

	//(experimental) calculate box fill data
	//bit=1 means box can fill into the hole
	unsigned short boxFillData[4][16]={};
	for(int boxIndex=0;boxIndex<boxCount;boxIndex++){
		const unsigned char w=boxWidth[boxIndex];
		const unsigned char h=boxHeight[boxIndex];

		for(int j=0;j<=height-h;j++){
			for(int i=0;i<=width-w;i++){
				const unsigned char bTarget=(boxTarget>>boxIndex)&1;
				bool b=true;
				for(int jj=0;jj<h && b;jj++){
					for(int ii=0;ii<w;ii++){
						if(bTarget){
							if(level.m_bTargetData[(j+jj)*level.m_nWidth+(i+ii)]==0){
								b=false;
								break;
							}
						}else{
							if((mapData[(unsigned char)((j+jj+1)*16+(i+ii+1))]&0x3)!=0x2){
								b=false;
								break;
							}
						}
					}
				}
				if(b) boxFillData[boxIndex][j]|=1<<i;
			}
		}
	}

	//calculate adjacency data
	//bit0-5: index, bit6-7: direction, 0xFF=empty
	unsigned char mapAdjacency[256][4];
	if(!TestSolver_CalculateAdjacancyData(mapAdjacency,mapData,blocks,blockCount)) return -2;

	//(experimental) 1x1-size target block static deadlock detection
	//0 means can't deadlock position
	unsigned short boxDeadlock[16]={};

	//(experimental) deadlock optimization and target block deadlock detection
	{
		//0=untested, 1=reachable, 2=unreachable (blocked)
		unsigned char reachable[256];
		//0=untested or unreachable (blocked), 1=reachable, 1 bit per direction
		unsigned char blockStateReachable[TestSolver_MaxBlockCount];

		//init
		for(int i=0;i<256;i++){
			unsigned char c;

			if((mapData[i]&0x3)==0x0) c=2; //???
			else{
				int idx=TestSolver_HitTestForBlocks(mapAdjacency,blocks,initState,i);
				c=(idx==-1)?1:((idx>>8)==0xA?2:0);
			}

			reachable[i]=c;
		}
		for(int i=0;i<blockCount;i++){
			unsigned char c=0;
			if(blocks[i].mask){
				c=1<<((unsigned char)(initState>>blocks[i].shift) & blocks[i].mask);
			}
			blockStateReachable[i]=c;
		}

		//iteration
		bool changed;
		do{
			changed=false;

			//check possible state of each block
			for(int i=0;i<blockCount;i++){
				unsigned char m=blocks[i].mask;
				if(m>0){
					unsigned char st=blockStateReachable[i];

					for(unsigned char d=0;d<=m;d++){
						if(st & (1<<d)){
							for(unsigned char ddd=0;ddd<2;ddd++){
								unsigned char d2=(d+1-(ddd<<1))&m;
								if((st & (1<<d2))==0){
									bool b=false;
									//check if player can push it (no double move check)
									for(int dir=0;dir<4;dir++){
										unsigned char pushPos=blocks[i].pos+dirLUT[dir];
										unsigned char playerPos=pushPos+dirLUT[(dir-1+(ddd<<1))&3];
										if(reachable[playerPos]==1 && (HitTestLUT[blocks[i].type+d] & (1<<dir))!=0
											&& (mapData[playerPos]&0x1)!=0 && (mapData[pushPos]&0x1)!=0)
										{
											b=true;
											break;
										}
									}
									//check if block can be rotated
									if(b){
										unsigned int n=LUT2[blocks[i].type+d][ddd];
										do{
											unsigned char checkPos=blocks[i].pos+((n&3)-2)+((n&12)-8)*4;
											if(reachable[checkPos]!=1){
												b=false;
												break;
											}
											n>>=4;
										}while(n&0xF);
										if(b){
											//we can rotate
											st|=(1<<d2);
											//update reachable area
											for(int ii=0;ii<4;ii++){
												unsigned char newPos=blocks[i].pos+dirLUT[ii];
												if((HitTestLUT[blocks[i].type+d] & (1<<ii))!=0 && (HitTestLUT[blocks[i].type+d2] & (1<<ii))==0
													&& reachable[newPos]==0)
												{
													reachable[newPos]=1;
												}
											}
										}
									}
								}
							}
						}
					}

					if(st!=blockStateReachable[i]){
						blockStateReachable[i]=st;
						changed=true;
					}
				}
			}
		}while(changed);

		//now all untested position is unreachable (i.e. value!=1)

		//check rotation block with type 0xA
		for(int i=0;i<blockCount;i++){
			if(!blocks[i].mask){
				bool b=true;
				for(int dir=0;dir<4;dir++){
					unsigned char newPos=blocks[i].pos+dirLUT[dir]+dirLUT[(dir+1)&3];
					if(reachable[newPos]!=1){
						b=false;
						break;
					}
				}
				if(b) blockStateReachable[i]=15;
			}
		}

		//simple target block deadlock detection
		for(int j=0;j<height;j++){ //< init
			for(int i=0;i<width;i++){
				if(level.m_bTargetData[j*width+i] && reachable[(unsigned char)((j+1)*16+(i+1))]==0x1){
					boxDeadlock[j]|=(1<<i);
				}
			}
		}
		for(;;){ //< iterate
			bool changed=false;

			for(int j=0;j<height;j++){
				for(int i=0;i<width;i++){
					if((boxDeadlock[j]>>i)&1){
						//block can push to this position, check its adjacent

						unsigned char idx=(j+1)*16+(i+1);

						//up
						if(j>=2 && reachable[(unsigned char)(idx-16)]==0x1
							&& reachable[(unsigned char)(idx-32)]==0x1
							&& ((boxDeadlock[j-1]>>i)&1)==0)
						{
							boxDeadlock[j-1]|=(1<<i);
							changed=true;
						}

						//down
						if(j<height-2 && reachable[(unsigned char)(idx+16)]==0x1
							&& reachable[(unsigned char)(idx+32)]==0x1
							&& ((boxDeadlock[j+1]>>i)&1)==0)
						{
							boxDeadlock[j+1]|=(1<<i);
							changed=true;
						}

						//left
						if(i>=2 && reachable[(unsigned char)(idx-1)]==0x1
							&& reachable[(unsigned char)(idx-2)]==0x1
							&& ((boxDeadlock[j]>>(i-1))&1)==0)
						{
							boxDeadlock[j]|=(1<<(i-1));
							changed=true;
						}

						//right
						if(i<width-2 && reachable[(unsigned char)(idx+1)]==0x1
							&& reachable[(unsigned char)(idx+2)]==0x1
							&& ((boxDeadlock[j]>>(i+1))&1)==0)
						{
							boxDeadlock[j]|=(1<<(i+1));
							changed=true;
						}
					}
				}
			}

			if(!changed) break;
		}

		//debug output
#if 0
		for(int i=0;i<blockCount;i++){
			printf("Block%d (%d,%d) %d",i,blocks[i].pos&0xF,blocks[i].pos>>4,blockStateReachable[i]);
			if((blockStateReachable[i] & (blockStateReachable[i]-1))==0) printf(" interlocked!");
			printf("\n");
		}
		for(int j=1;j<=height;j++){
			for(int i=1;i<=width;i++){
				printf("%d",reachable[j*16+i]);
			}
			printf("\n");
		}
#endif

		//target block deadlock debug
#if 0
		printf("Target block deadlock:\n");
		for(int j=0;j<height;j++){
			for(int i=0;i<width;i++){
				printf("%d",(boxDeadlock[j]>>i)&1);
			}
			printf("\n");
		}
#endif

		//output extended data
		if(ed){
			memcpy(ed->reachable,reachable,sizeof(reachable));
			memcpy(ed->blockStateReachable,blockStateReachable,sizeof(blockStateReachable));
		}

		//remove deadlocked block
		int deadlockBlockCount=0;
		TestSolverStateType newState=initState&((TestSolverStateType(1)<<newSize)-1);

		for(int i=0;i<blockCount;i++){
			if((blockStateReachable[i] & (blockStateReachable[i]-1))==0){
				//blocked
				unsigned char ht=HitTestLUT[blocks[i].type+((initState>>blocks[i].shift)&blocks[i].mask)];
				for(int ii=0;ii<4;ii++){
					if(ht & (1<<ii)) mapData[(unsigned char)(blocks[i].pos+dirLUT[ii])]&=~0x3;
				}
				deadlockBlockCount++;
			}else{
				int ii=i-deadlockBlockCount;
				if(blocks[i].mask){
					newState|=((TestSolverStateType)((initState>>blocks[i].shift)&blocks[i].mask))<<newSize;
					blocks[i].shift=newSize;
					newSize+=(blocks[i].type==0x8)?1:2;
				}
				if(deadlockBlockCount>0) blocks[ii]=blocks[i];
			}
		}

		if(ed) ed->deadlockBlockCount=deadlockBlockCount;

		//recalculate adjacency data
		if(deadlockBlockCount>0){
#ifdef _DEBUG
			printf("[TestSolver] Debug: Optimized out %d rotate block(s)\n",deadlockBlockCount);
#endif
			initState=newState;
			stateSize=newSize;
			blockCount-=deadlockBlockCount;
			if(!TestSolver_CalculateAdjacancyData(mapAdjacency,mapData,blocks,blockCount)) return -2;
		}
	}

	//debug
#ifdef  _DEBUG
	printf("[TestSolver] Debug: StateSize=%d (XSize=%d,YSize=%d), InitState="
#ifdef WIN32
		"%016I64X"
#else
		"%016LX"
#endif
		"\n",stateSize,playerXSize,playerYSize,
		initState
		);
#endif

	//================ run solver
#ifdef USE_CUSTOM_HASHTREE
	AllocateOnlyHashRedBlackTree<TestSolverNode,8> nodeMap;
#else
	std::map<TestSolverStateType,TestSolverNode> nodeMap;
#endif
	int currentIndex=0;
	TestSolverNode* currentNode=NULL,*tail=NULL;

	if(rec) rec->clear();

	const unsigned char playerXMask=(1<<playerXSize)-1;
	const unsigned char playerYMask=(1<<playerYSize)-1;

	const unsigned int exitState=((exitPos&0xF)-1)|((((exitPos>>4)&0xF)-1)<<playerXSize);

	//create initial node
	{
		unsigned char playerRemaining=level.m_nPlayerCount;

		unsigned char shift=0;
		for(int i=0;i<level.m_nPlayerCount;i++){
			unsigned char x=(initState>>shift)&playerXMask,y=(initState>>(shift+playerXSize))&playerYMask;
			unsigned char pos=(y+1)*16+(x+1);
			if(mapData[pos]&0x4){
				initState=(initState&~(((TestSolverStateType(1)<<(playerXSize+playerYSize))-1)<<shift))
					|(TestSolverStateType(exitState)<<shift);
				playerRemaining--;
			}
			shift+=playerXSize+playerYSize;
		}

		if(playerRemaining==0) return 1;

		initState=TestSolver_SortPosition(level.m_nPlayerCount,playerXSize+playerYSize,0,initState);

		//check if box init position is impossible
		for(int boxIndex=0;boxIndex<boxCount;boxIndex++){
			if((useDeadlock>>boxIndex)&1){
				unsigned char boxX=(initState>>boxXShift[boxIndex])&boxXMask[boxIndex];
				unsigned char boxY=(initState>>boxYShift[boxIndex])&boxYMask[boxIndex];
				if(((boxDeadlock[boxY]>>boxX)&1)==0) return 0;
			}
		}

		TestSolverNode node={NULL,NULL,initState};
#ifdef USE_CUSTOM_HASHTREE
		nodeMap.find_or_insert(node,&currentNode);
#else
		currentNode=&(nodeMap.insert(
			std::pair<TestSolverStateType,TestSolverNode>(initState,node)).first->second);
#endif
		tail=currentNode;
	}

	do{
		const TestSolverNode &node=*currentNode;

		unsigned char playerRemaining=level.m_nPlayerCount;

		unsigned char pos[4];

		unsigned char boxX[4],boxY[4],boxPos[4];
		unsigned char boxFilled=0;
		for(int boxIndex=0;boxIndex<boxCount;boxIndex++){
			boxX[boxIndex]=(node.state>>boxXShift[boxIndex])&boxXMask[boxIndex];
			boxY[boxIndex]=(node.state>>boxYShift[boxIndex])&boxYMask[boxIndex];
			unsigned char p=boxPos[boxIndex]=(boxY[boxIndex]+1)*16+(boxX[boxIndex]+1);
			bool bFilled=((boxFillData[boxIndex][boxY[boxIndex]]>>boxX[boxIndex])&0x1)!=0;

			if((boxTarget>>boxIndex)&1){
				if(!bFilled) playerRemaining=0xFF;
				bFilled=false;
			}

			//adhoc!!! slow!!!
			const unsigned char w=boxWidth[boxIndex],h=boxHeight[boxIndex];

			if(bFilled){
				boxFilled|=1<<boxIndex;

				for(int j=0;j<h;j++,p+=16){
					for(int i=0;i<w;i++) mapData[(unsigned char)(p+i)]|=0x3;
				}
			}else{
				for(int j=0;j<h;j++,p+=16){
					for(int i=0;i<w;i++) mapData[(unsigned char)(p+i)]|=(1+boxIndex)<<3;
				}
			}
		}

		unsigned char shift=0;
		for(int i=0;i<level.m_nPlayerCount;i++){
			unsigned char x=(node.state>>shift)&playerXMask;
			unsigned char y=(node.state>>(shift+playerXSize))&playerYMask;
			if((pos[i]=(y+1)*16+(x+1))==exitPos) playerRemaining--;
			shift+=playerXSize+playerYSize;

			//adhoc????
			if(mapData[pos[i]]==0x3) mapData[pos[i]]=0x0;
		}

		//DEBGU
		//printf("%d->%d p %d %d b %d %d\n",node.parent,currentIndex,(pos[0]-17)&0xF,(pos[0]-17)>>4,boxX,boxY);

		//for each player
		shift=0;
		for(int currentPlayer=0;currentPlayer<level.m_nPlayerCount;currentPlayer++,shift+=playerXSize+playerYSize){
			if(pos[currentPlayer]==exitPos) continue;

			//adhoc???
			if(mapData[pos[currentPlayer]]==0x0) mapData[pos[currentPlayer]]=0x3;

			//for each direction
			for(int d=0;d<4;d++){
				const char dir=dirLUT[d];
				unsigned char newPos=pos[currentPlayer]+dir;
				unsigned char c=mapData[newPos];
				unsigned char boxPushedIndex=0; // boxPushedIndex-1 is real index

				//check blocked exit
				if((c&0x4) && (playerRemaining&0x80)) c=0;

				//check push box
				if((c&0x1) && ((c>>3)&0x7)){
					unsigned char boxIndex=((c>>3)&0x7)-1;
					unsigned char w=boxWidth[boxIndex],h=boxHeight[boxIndex];

					unsigned char checkPos=boxPos[boxIndex]+
						(d==0?-16:(d==1?-1:(d==2?(h*16):w)));
					unsigned char checkStep=(d&1)?16:1;
					unsigned char checkCount=(d&1)?h:w;

					//check if it's blocked
					for(;checkCount;checkPos+=checkStep,checkCount--){
						if((mapData[checkPos]&(0x2|0x8|0x10|0x20))!=0x2
							|| TestSolver_HitTestForBlocks(mapAdjacency,blocks,node.state,checkPos)!=-1)
						{
							c=0;
							break;
						}
					}

					//check if new position is deadlocked
					if(c && ((useDeadlock>>boxIndex)&1)){
						const unsigned char p=boxPos[boxIndex]-17+dir;
						if(((boxDeadlock[p>>4]>>(p&15))&1)==0) c=0;
					}

					//over
					if(c) boxPushedIndex=boxIndex+1;
				}

				if(c&0x1){
					//it's movable, now check for blocks
					int blockIdx=TestSolver_HitTestForBlocks(mapAdjacency,blocks,node.state,newPos);
					TestSolverStateType newState=node.state & ~(((((TestSolverStateType)1)<<(playerXSize+playerYSize))-1)<<shift);

					if(blockIdx!=-1){
						TestRotateBlock block=blocks[blockIdx&0x3F];
						int tmp=d-((blockIdx>>6)&3);
						if(tmp&1){
							//check if block can be rotated
							unsigned int n=LUT2[(blockIdx>>8)&0xF][(tmp&2)>>1];
							bool b=true;

							do{
								unsigned char checkPos=block.pos+((n&3)-2)+((n&12)-8)*4;

								if((mapData[checkPos]&(0x2|0x8|0x10|0x20))!=0x2
									|| TestSolver_HitTestForBlocks(mapAdjacency,blocks,node.state,checkPos)!=-1)
								{
									b=false;
									break;
								}

								n>>=4;
							}while(n&0xF);

							if(b){
								//can be rotated, update new state
								unsigned char newDir=0;
								if(block.mask){
									newDir=((node.state>>block.shift)+(1-(tmp&2)))&block.mask;
									newState&=~(((TestSolverStateType)block.mask)<<block.shift);
									newState|=((TestSolverStateType)newDir)<<block.shift;
								}

								//check if we should move twice
								if(HitTestLUT[block.type+newDir] & (1<<((blockIdx>>6)&3))){
									newPos+=dir;
									//no need to check if blocked by box because it will block rotation
									if(((c=mapData[newPos])&0x1)!=0 &&
										((c&0x4)==0 || (playerRemaining&0x80)==0) /* check blocked exit */
										) blockIdx=-1;
								}else{
									blockIdx=-1;
								}
							}
						}
					}

					if(blockIdx==-1){
						//FIXME: in sokoban mode there is one more move
						if(((c&0x4) && playerRemaining==1) || (exitCount==0 && boxTarget && playerRemaining<=level.m_nPlayerCount)){
							//get player position of each step
							std::vector<PlayerPosition> positions;
							PlayerPosition newPos={exitPos,exitPos,exitPos,exitPos};
							positions.push_back(newPos);

							TestSolverNode *p=currentNode;
							do{
								const TestSolverNode& node=*p;

								PlayerPosition pos;
								unsigned char shift=0;
								for(int i=0;i<level.m_nPlayerCount;i++){
									unsigned char x=(node.state>>shift)&playerXMask,y=(node.state>>(shift+playerXSize))&playerYMask;
									pos[i]=y*16+x;
#ifdef USE_SOLUTION_REACHABLE
									if(ed && pos[i]!=exitPos /* ??? */) ed->solutionReachable[pos[i]]=1;
#endif
									pos[i]+=17;

									shift+=playerXSize+playerYSize;
								}

								//determine which player is moved
								int index=-1,newIndex=-1,newIndexMask=(1<<level.m_nPlayerCount)-1;
								for(int i=0;i<level.m_nPlayerCount;i++){
									bool b=true;
									for(int j=0;j<level.m_nPlayerCount;j++){
										if(pos[i]==newPos[j] && (newIndexMask&(1<<j))!=0){
											newIndexMask&=~(1<<j);
											b=false;
											break;
										}
									}
									if(b){
										assert(index<0);
										index=i;
									}
								}
								assert(newIndexMask && (newIndexMask&(newIndexMask-1))==0);
								switch(newIndexMask){
								case 1: newIndex=0; break;
								case 2: newIndex=1; break;
								case 4: newIndex=2; break;
								case 8: newIndex=3; break; //no more than 4 players
								}

								//update position
								newPos[newIndex]=pos[index];
								positions.push_back(newPos);

								p=node.parent;
							}while(p);

							//generate solution
							if(rec){
								int prevPlayer=-1;
								for(int i=positions.size()-1;i>0;i--){
									int currentPlayer=-1;

									for(int j=0;j<level.m_nPlayerCount;j++){
										if(positions[i][j]!=positions[i-1][j]){
											currentPlayer=j;
											break;
										}
									}
									assert(currentPlayer>=0);

									unsigned char pos=positions[i][currentPlayer],
										newPos=positions[i-1][currentPlayer];

									//check if go to exit
									if(newPos==exitPos){
										for(int i=0;i<4;i++){
											char d=dirLUT[i];
											if(mapData[(unsigned char)(pos+d)]&0x4){
												newPos=pos+d;
												break;
											}
										}
									}

									if(currentPlayer!=prevPlayer){
										if(level.m_nPlayerCount>1){
											//we need to switch player
											rec->append(str(MyFormat("(%d,%d)")<<(pos&0xF)<<((pos>>4)&0xF)));
										}
										prevPlayer=currentPlayer;
									}

									//generate one step
									rec->push_back((newPos>pos)?(newPos>pos+2?'S':'D'):(newPos<pos-2?'W':'A'));
								}
							}

							//generate extended data
							if(ed){
								char blockUsed[TestSolver_MaxBlockCount]={};

								TestSolverNode *p=currentNode;
								TestSolverStateType st=newState;
								ed->moves=positions.size()-1;
								ed->pushes=0;

								unsigned char shift=(playerXSize+playerYSize)*level.m_nPlayerCount;

								do{
									const TestSolverNode& node=*p;

									//check if some block is moved
									st^=node.state;
									if(st>>shift){
										ed->pushes++;
										for(int i=0;i<blockCount;i++){
											if(((unsigned char)(st>>blocks[i].shift)) & blocks[i].mask){
												blockUsed[i]=1;
												break;
											}
										}
									}

									p=node.parent;
									st=node.state;
								}while(p);

								int count=0;
								for(int i=0;i<blockCount;i++){
									if(blockUsed[i]) count++;
								}

								ed->state.nAllNodeCount=nodeMap.size();
								ed->state.nOpenedNodeCount=currentIndex;
								ed->blockUsed=count;
							}

							//debug
#ifdef _DEBUG
							printf("[TestSolver] Debug: Solution found. Nodes=%d (Opened=%d), Step=%d\n",nodeMap.size(),currentIndex,positions.size()-1);
#endif

#ifdef SOLVER_PROFILING
							printf("Time=%dms\n",SDL_GetTicks()-ttt);
#endif

							return 1;
						}

						//add node
						if(c&0x4) newPos=exitPos;
						newPos-=17;
						newState=TestSolver_SortPosition(level.m_nPlayerCount,
							playerXSize+playerYSize,0,
							newState|TestSolverStateType((newPos&playerXMask)|(((newPos>>4)&playerYMask)<<playerXSize))<<shift);
						if(boxPushedIndex){
							boxPushedIndex--;
							unsigned char newBoxPos=boxPos[boxPushedIndex]+dir-17;
							newState=(newState&~TestSolverStateType((TestSolverStateType(boxXMask[boxPushedIndex])<<boxXShift[boxPushedIndex])
								|(TestSolverStateType(boxYMask[boxPushedIndex])<<boxYShift[boxPushedIndex])))
								|(TestSolverStateType(newBoxPos&boxXMask[boxPushedIndex])<<boxXShift[boxPushedIndex])
								|(TestSolverStateType((newBoxPos>>4)&boxYMask[boxPushedIndex])<<boxYShift[boxPushedIndex]);
							if(boxStateSize) newState=TestSolver_SortPosition(boxCount,boxStateSize,boxXShift[0],newState);
						}
						TestSolverNode newNode={currentNode,NULL,newState},*p=NULL;
#ifdef USE_CUSTOM_HASHTREE
						if(!nodeMap.find_or_insert(newNode,&p)){
#else
						if(nodeMap.find(newState)==nodeMap.end()){
							p=&(nodeMap.insert(
								std::pair<TestSolverStateType,TestSolverNode>(newState,newNode)).first->second);
#endif
							tail->next=p;
							tail=p;
						}
					}
				}
			}

			//adhoc???
			if(mapData[pos[currentPlayer]]==0x3) mapData[pos[currentPlayer]]=0x0;
		}

		//ad-hoc???
		for(int i=0;i<level.m_nPlayerCount;i++){
			if(mapData[pos[i]]==0x0) mapData[pos[i]]=0x3;
		}

		//adhoc!!! slow!!!
		for(int boxIndex=0;boxIndex<boxCount;boxIndex++){
			unsigned char p=boxPos[boxIndex];
			const unsigned char w=boxWidth[boxIndex],h=boxHeight[boxIndex];

			if((boxFilled>>boxIndex)&1){
				for(int j=0;j<h;j++,p+=16){
					for(int i=0;i<w;i++) mapData[(unsigned char)(p+i)]=0x2;
				}
			}else{
				for(int j=0;j<h;j++,p+=16){
					for(int i=0;i<w;i++) mapData[(unsigned char)(p+i)]&=~(0x8|0x10|0x20);
				}
			}
		}

#ifdef _DEBUG
#define PROGRESS_MASK 0x7FF
#else
#define PROGRESS_MASK 0x7FFF
#endif

		//show progress
		if(((++currentIndex) & PROGRESS_MASK)==0 && callback){
#ifndef SOLVER_PROFILING
			LevelSolverState progress={
				nodeMap.size(),
				currentIndex,
			};
			if(callback(userData,progress)) return -1;
#endif
		}

		//next node
		currentNode=currentNode->next;
	}while(currentNode);

	//generate extended data
	if(ed){
		ed->state.nAllNodeCount=ed->state.nOpenedNodeCount=currentIndex;
		//TODO: number of blocks reachable
	}

	//debug
#ifdef _DEBUG
	printf("[TestSolver] Debug: Solution not found. Nodes=%d\n",nodeMap.size());
#endif

#ifdef SOLVER_PROFILING
	printf("Time=%dms\n",SDL_GetTicks()-ttt);
#endif

	return 0;
}

int RunSolver(SolverType type,const PuzzleBoyLevel& level,u8string* rec,void* userData,LevelSolverCallback callback){
	switch(type){
	case TEST_SOLVER: return TestSolver_SolveIt(level,rec,userData,callback);
	default: return -2;
	}
}
