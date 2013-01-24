#include "ColorBoard.h"

#include<stack>
#include<utility>

#define x first
#define y second

typedef std::pair<int,int> IP;

static const int dx[]={1,0,-1,0};
static const int dy[]={0,1,0,-1};


// Flood filling functions
void ColorBoard::ff1(int x,int y,int o)
{
    if(x<0||x>=getWidth()||y<0||y>=getHeight())
        return;
    if(ownage[x][y]!=NO_PLAYER)return;
    if(color[x][y]!=getPlayerColor(o))
        return;
    ownage[x][y]=o;
    for(int i=0;i<4;i++){
        ff1(x+dx[i],y+dy[i],o);
    }
}
void ColorBoard::ff2(int x,int y,int p,int oc,int nc){

    if(x<0||x>=getWidth()||y<0||y>=getHeight())
        return;
    if(color[x][y]!=oc){
        return;
    }
    color[x][y]=nc;
    for(int i=0;i<4;i++){
        ff2(x+dx[i],y+dy[i],p,oc,nc);
    }
}

ColorBoard::ColorBoard(int w,int h):
    color(makeIVV(w,h)),
    ownage(makeIVV(w,h))
{
	gameEnded=-1;
	turn=rand()<RAND_MAX/2;
    width=w;
    height=h;
    for(int i=0;i<getWidth();i++){
        for(int j=0;j<getHeight();j++){
            color[i][j]=rand()%5;
        }
    }

    if(color[getWidth() - 1][getHeight() - 1] == color[0][0]) {
        ++color[0][0];
        color[0][0] %= 5;
    }
	while(color[1][0]==color[0][0]) color[1][0]=rand()%5;
	while(color[0][1]==color[0][0]) color[0][1]=rand()%5;
	while(color[getWidth()-2][getHeight()-1]==color[getWidth()-1][getHeight()-1]) color[getWidth()-2][getHeight()-1]=rand()%5;
	while(color[getWidth()-1][getHeight()-2]==color[getWidth()-1][getHeight()-1]) color[getWidth()-1][getHeight()-2]=rand()%5;
	

    for(int i=0;i<getWidth();i++){
        for(int j=0;j<getHeight();j++){
            ownage[i][j]=NO_PLAYER;
        }
    }

    ff1(0,0,0);
    ff1(getWidth()-1,getHeight()-1,1);
}
void ColorBoard::makeMove(int player,int newColor){
	if(player!=turn) {
		return;
	}
	if(gameEnded>=0) {
		return;
	}
    IP startCorner=
        player==0?IP(0,0):IP(getWidth()-1,getHeight()-1);
    int oldColor = getColor(startCorner.x,startCorner.y);

    ff2(startCorner.x,startCorner.y,player,oldColor,newColor);

	for (int i=0; i<getWidth(); i++) {
		for(int j=0;j<getHeight();j++){
			ownage[i][j]=-1;
		}
	}
	ff1(0,0,0);
    ff1(getWidth()-1,getHeight()-1,1);
	turn=(turn+1)%2;
	if(getArea(0)>getWidth()*getHeight()/2) {
		gameEnded=0;
	}
	else if(getArea(1)>getWidth()*getHeight()/2) {
		gameEnded=1;
	}
	else if(getArea(0)+getArea(1)==getWidth()*getHeight()) {
		gameEnded=2;
	}
	
}

int ColorBoard::getTurn() const{
	return turn;
}

int ColorBoard::getArea(int player)const{
    int ret = 0;
    for(int i=0;i<getWidth();i++)
        for(int j=0;j<getHeight();j++)
        {
            if(ownage[i][j]==player)
                ret++;
        }

    return ret;
}
