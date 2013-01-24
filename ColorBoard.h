#ifndef COLOR_BOARD_HPP
#define COLOR_BOARD_HPP

#include<cstdlib>
#include<vector>

class ColorBoard {
    public:
    typedef std::vector<int> IVZ;
    typedef std::vector<IVZ> IVV;
    inline static IVV makeIVV(int w,int h){
        return IVV(w,IVZ(h));
    }
    private:
	
    int width,height;
    IVV color;
    IVV ownage;
	int gameEnded;
    int turn;
    void ff1(int x,int y,int o);
    void ff2(int x,int y,int p,int oc,int nc);
    public: 
        ColorBoard(int w,int h);

        void makeMove(int player,int color);
        int getColor(int x,int y)const{
			if(x<0||y<0||x>=getWidth()||y>=getHeight()) return -1;
            return color[x][y];
        }
        int getPlayerColor(int p)const{
            if(p==0)
                return getColor(0,0);
            return getColor(getWidth()-1,getHeight()-1);
        }

        static const int NO_PLAYER=-1;
        int getOwner(int x,int y)const{
            return ownage[x][y];
        }

        int getArea(int player)const;
        int getWidth()const{
            return width;
        }
        int getHeight()const{
            return height;
        }
	int getGameEnded()const {
		return gameEnded;
	}
		int getTurn()const;
};

#endif

