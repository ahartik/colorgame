#include<iomanip>
#include<iostream>
#include "ColorAI.h"
#include<cstring>
#include<cstdlib>

const int WIN = 1e8;
const int LOSE = -1e8;

static unsigned hash_combine(unsigned seed,unsigned h)
{
    seed ^= 17*h + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}

static inline void sort_small(int* mv,int k,int* score){
    for(int j=0;j<k-1;j++){
        int bi=j;
        int bc=score[mv[j]];
        for(int i=j+1;i<k;i++){
            if(score[mv[i]]>bc){
                bi=i;
                bc=score[mv[i]];
            }
        }
        std::swap(mv[bi],mv[j]);
    }
}

static const int HASH_SIZE=10*1024*1024;

static inline int other(int p){
    return p^1;
}

using namespace std;


static const int HW = 128;
static unsigned zt[HW][HW][2];
static int dx[4]={0,1,0,-1};
static int dy[4]={1,0,-1,0};

static void initHashes(){
    for(int i=0;i<HW;i++){
        for(int j=0;j<HW;j++){
            for(int k=0;k<2;k++){
                zt[i][j][k]=(rand()<<16)^(rand()>>16)^(rand()<<8)^(rand()>>8)^rand();
            }
        }
    }
}

inline unsigned cellHash(int x,int y){
    return zt[x][y][0];
}
inline unsigned cellHash2(int x,int y){
    return zt[x][y][1];
}


struct ColorNode{
    unsigned hash;
    unsigned hash2;
    int count;
    int color;
    int owner;
    bool operator==(const ColorNode& o)const{
        return 
            hash==o.hash&&
            hash2==o.hash2&&
            count==o.count&&
            color==o.color&&
            owner==o.owner;
    }
};

std::ostream& operator<<(std::ostream& out,const ColorNode& c){
    out<<"count = "<<c.count<<";";
    out<<"color = "<<c.color<<";";
    out<<"owner = "<<c.owner<<";";
    return out;
}

#include<map>
#include<cassert>
#include<tr1/array>

typedef std::map<int,int> IntMap;


bool map_eq(const IntMap& a,const IntMap& b){
    if(a.size()!=b.size())return false;
    for(IntMap::const_iterator it=a.begin();it!=a.end();++it){
        IntMap::const_iterator f = b.find(it->first);
        if(f==b.end())return false;
        if(f->second!=it->second)return false;
    }
    return true;
}

struct MergeUndo{
    int to,n;
};
const int DRAW = 2;

struct MoveUndo{
    std::vector<MergeUndo> merges;
    int player;
    int color;
};

typedef std::tr1::array<int,3> MoveList;
int ml=0;

struct ColorGraph{
    int totalCount;
    std::vector<ColorNode> v;
    std::vector<IntMap> e;
    int playerNode[2];

    ColorGraph(const ColorBoard& b){
        this->totalCount = b.getWidth()*b.getHeight();

        struct FF{
            const ColorBoard& colors;
            int w;
            int h;
            short node[50][50];
            void fill(int x,int y,ColorNode& rc,int n){
                if(x<0||y<0||x>=w||y>=h)return;
                if(colors.getColor(x,y)!=rc.color)return;
                if(node[x][y]!=-1)return;
                node[x][y]=n;
                rc.count++;
                rc.hash^=cellHash(x,y);
                rc.hash2^=cellHash2(x,y);
                for(int k=0;k<4;k++)
                    fill(x+dx[k],y+dy[k],rc,n);
            }
            FF(const ColorBoard& z):colors(z),w(z.getWidth()),h(z.getHeight()){
                memset(node,-1,sizeof(node));
            }
        };

        FF ff(b);
        int w = ff.w;
        int h = ff.h;
        int fill=0;
        for(int i=0;i<w;i++){
            for(int j=0;j<h;j++){
                if(ff.node[i][j]==-1){
                    this->v.push_back(ColorNode());
                    this->v.back().color=b.getColor(i,j);
                    this->v.back().owner=-1;
                    ff.fill(i,j,this->v.back(),fill++);
                }
            }
        }

        this->e.resize(fill);

        this->playerNode[0]=0;
        this->playerNode[1]=ff.node[b.getWidth()-1][b.getHeight()-1];
        this->v[this->playerNode[0]].owner=0;
        this->v[this->playerNode[1]].owner=1;
        for(int i=0;i<w;i++){
            for(int j=0;j<h;j++){
                int n=ff.node[i][j];
                for(int k=0;k<4;k++)
                {
                    int ni = i+dx[k];
                    int nj = j+dy[k];
                    if(ni<0||nj<0||ni>=w||nj>=h)continue;
                    int n2=ff.node[ni][nj];

                    if(n2==n)continue;

                    this->inc(n,n2,1);
                }
            }
        }
    }


    void inc(int f,int t,int a){
        int& ft = e[f][t];
        ft+=a;
        if(ft<=0)
            e[f].erase(t);
    }

    MergeUndo merge(int to,int n){
        ml++;
        MergeUndo undo;
        undo.to=to;
        undo.n = n;

        v[to].count+=v[n].count;
        v[to].hash^=v[n].hash;

        v[to].hash2^=v[n].hash2;

        for(IntMap::iterator it=e[n].begin();it!=e[n].end();++it){
            int et = it->first;

            inc(to,et,it->second);
            inc(et,to,it->second);

            inc(et,n,-it->second);
        }
        return undo;
    }
    void mergeD(int to,int n){
        merge(to,n);
        e[n].clear();
    }
    

    void undoMerge(MergeUndo u){

        int n = u.n;
        int to = u.to;
        ml--;
        v[to].count-=v[n].count;
        v[to].hash^=v[n].hash;
        v[to].hash2^=v[n].hash2;

        for(IntMap::iterator it=e[n].begin();it!=e[n].end();++it){
            int et = it->first;

            inc(to,et,-it->second);
            inc(et,to,-it->second);
            inc(et,n,it->second);
        }
    }

    int ply_;
    MoveUndo makeMove(int player,int color){
        ply_++;
        MoveUndo ret;

        ret.player=player;
        int n = playerNode[player];
        ret.color = v[n].color;

        std::vector<int> merges;

        for(IntMap::iterator it=e[n].begin();it!=e[n].end();++it){
            int o = it->first;
            if(v[o].color==color)
                merges.push_back(o);
        }

        for(size_t i=0;i<merges.size();i++){
            ret.merges.push_back(merge(n,merges[i]));
        }
        v[n].color=color;
        return ret;
    }

    int ply(){
        return ply_;
    }

    void makeUndo(const MoveUndo& u){
        ply_--;
        for(int i=u.merges.size()-1;i>=0;i--){
            undoMerge(u.merges[i]);
        }
        v[playerNode[u.player]].color=u.color;
    }

    MoveList getMoves()const{
        MoveList ret;
        int i=0;
        for(int c=0;c<5;c++){
            bool used=false;
            for(int z=0;z<2;z++)
                if(this->v[playerNode[z]].color==c){
                    used=true;
                    break;
                }
            if(!used){
                ret[i]=c;
                i++;
            }
        }
        return ret;
    }

    int sortMoves(MoveList& m,int p){
        IntMap& ed=e[playerNode[p]];
        int cnt[5]={};
        for(IntMap::iterator it=ed.begin();it!=ed.end();++it){
            int o=it->first;
            cnt[v[o].color]+=v[o].count;
        }
        for(int j=0;j<2;j++){
            int bi=j;
            int bc=cnt[m[j]];
            for(int i=j+1;i<3;i++){
                if(cnt[m[i]]>bc){
                    bi=i;
                    bc=cnt[m[i]];
                }
            }
            std::swap(m[bi],m[j]);
        }
        int r=3;
        while(r>0&&(cnt[m[r-1]]==0))
            r--;
        if(r<=0)r=1;
        return r;
    }
    const ColorNode& pn(int p)const{
        return v[playerNode[p]];
    }
    unsigned hash1()const{
        unsigned h = hash_combine(pn(0).color<<3,pn(1).color<<10);
        h=hash_combine(h,pn(0).hash);
        h=hash_combine(h,pn(1).hash2);
        return h;
    }
    unsigned hash2()const{
        unsigned h = hash_combine(pn(0).color<<4,pn(1).color<<5);
        h=hash_combine(h,pn(0).hash2);
        h=hash_combine(h,pn(1).hash);
        return h;
    }
    int getWinner()const
    {
        int c0=pn(0).count;
        int c1=pn(1).count;

        if(c0>totalCount/2){
            return 0;
        }
        if(c1>totalCount/2){
            return 1;
        }
        if(c1==totalCount/2&&c0==c1){
            return DRAW;
        }
        return -1;
    }

};



typedef std::vector<int> IV;
typedef std::vector<IV> IVV;

void printColorBlock(int color){
    std::cout<<"["<<20+color<<";"<<40+color<<"m";
    std::cout<<"  ";
    std::cout<<"[0;0m";
}

void printBoard(ColorBoard& b){
    for(int i=0;i<b.getHeight();i++){
        for(int j=0;j<b.getWidth();j++){
            int c = b.getColor(i,j);
            printColorBlock(c);
        }
        std::cout<<"\n";
    }
    std::cout<<"\n";
}

void printGraph(ColorGraph& g){
    for(size_t i=0;i<g.v.size();i++)
        std::cout<<g.v[i]<<"\n";
    for(size_t i=0;i<g.e.size();i++){
        for(IntMap::iterator it=g.e[i].begin();it!=g.e[i].end();++it){
            std::cout<<it->first<<" ";
        }
        std::cout<<"\n";
    }

}

enum TPTType {
    EXACT,
    UPPERBOUND,
    LOWERBOUND
}__attribute ((packed));

struct TPTItem{
    unsigned hash1;
    unsigned hash2;
    int value;
    short ply;
    char depth;
    char bestMove;
    TPTType type;
}__attribute ((packed));

static const size_t HASH_COUNT = ((HASH_SIZE/2)/sizeof(TPTItem))-1;

static TPTItem transpos_table[HASH_COUNT][2];

TPTItem* getTPT(const ColorGraph& g,int player){
    unsigned h1 = g.hash1();
    unsigned h2 = g.hash2();
    TPTItem* ret = &transpos_table[h1%HASH_COUNT][player];
    if(ret->hash1==h1&&ret->hash2==h2){
        return ret;
    }
    return NULL;
}
int tp_cols=0;//counter for collisions

void storeTPT(const ColorGraph& g,int player,const TPTItem& s){
    int h1 = g.hash1();
    TPTItem* ret = &transpos_table[h1%HASH_COUNT][player];
    bool okToSave=false;
    if(ret->hash1==0&&ret->hash2==0){
        //most probably empty 
        okToSave=true;
    }else if(ret->depth<=s.depth || ret->ply+3 < s.ply){
        okToSave=true;
        tp_cols++;
    }else{
        tp_cols++;
    }

    if(okToSave)
        *ret=s;

}

#include<queue>

struct QN{
    int p;
    int n;
    int d;
    QN(){};
    QN(int a,int b,int c):
        p(a),n(b),d(c){}
};
const int NOT_REACHABLE=100000;

void bfsDists(ColorGraph& g,int p,std::vector<int>& save){
    if(save.size()!=g.v.size())
        save.resize(g.v.size());
    std::fill(save.begin(),save.end(),NOT_REACHABLE);
    std::queue<QN> q;
    q.push( QN(p,g.playerNode[p],0) );
    assert(g.pn(p).owner==p);
    while(q.size()){
        QN n=q.front();
        q.pop();
        int no = g.v[n.n].owner;
        if(no!=-1&&no!=p)continue;
        if(save[n.n]<=n.d)continue;
        save[n.n]=n.d;

        for(IntMap::iterator it=g.e[n.n].begin();it!=g.e[n.n].end();++it){
            if(save[it->first]>n.d+1){
                QN n2(n.p,it->first,n.d+1);
                q.push(n2);
            }
        }
    }
}

static int eval(ColorGraph& g,int p){

    int o=other(p);
    static std::vector<int> dist[2];

    int ret=0;

    int realCnt[2]={g.pn(0).count,g.pn(1).count};

    bfsDists(g,0,dist[0]);
    bfsDists(g,1,dist[1]);

    for(size_t i=0;i<dist[0].size();i++){
        int m=dist[p][i];
        int e=dist[o][i];
        int count = g.v[i].count;
        if(m<e){
            ret+=count;
            if(e==NOT_REACHABLE && g.v[i].owner==-1)
                realCnt[p]+=count;
        }
        if(m>e){
            ret-=count;
            if(m==NOT_REACHABLE && g.v[i].owner==-1)
                realCnt[o]+=count;
        }
    }

    if(realCnt[o]>g.totalCount/2)return LOSE+realCnt[p]-realCnt[o];
    if(realCnt[p]>g.totalCount/2)return WIN+realCnt[p]-realCnt[o];

    return ret;
}

int total_pos = 0;
int tp_hits=0;
int negamax(ColorGraph& g,int p,int depth,int alpha,int beta,int& bestMove){

    static const int BETTER_MOVEORDER_DEPTH=8;

    int w=g.getWinner();

    total_pos++;

    if(w>=0){
        if(w==DRAW)return 0;

        assert(g.pn(p).count!=g.pn(other(p)).count);
        if(w==p){
            return WIN+g.pn(p).count-g.pn(other(p)).count;
        }else{
            return LOSE+g.pn(p).count-g.pn(other(p)).count;
        }
    }
    if(depth==0)
        return eval(g,p);


    TPTItem* tp = getTPT(g,p);
    if(tp!=NULL){
        if(tp->depth>=depth){
            tp_hits++;
            switch(tp->type){
                case LOWERBOUND:
                    alpha = std::max(alpha,tp->value);
                    break;
                case UPPERBOUND:
                    beta=std::min(beta,tp->value);
                    break;
                case EXACT:
                    bestMove=tp->bestMove;
                    return tp->value;
            };
            if(alpha>=beta){
                bestMove=tp->bestMove;
                return alpha;
            }
        }
    }

    int orig_alpha=alpha;

    MoveList mv=g.getMoves();
    int numMoves=3;
    
    if(depth>=BETTER_MOVEORDER_DEPTH){
        numMoves=g.sortMoves(mv,p);
        int score[5]={};

        for(int i=0;i<numMoves;i++){
            MoveUndo u = g.makeMove(p,mv[i]);
            score[mv[i]] = eval(g,p);
            g.makeUndo(u);
        }
        sort_small(mv.data(),numMoves,score);
        static bool fp=true;
        if(fp){
            std::cout<<"sort \n";
            for(int i=0;i<numMoves;i++){
                std::cout<<mv[i]<<" "<<score[mv[i]]<<"\n";
            }
            std::cout<<"\n";
            fp=false;
        }
    }else{
        numMoves=g.sortMoves(mv,p);
    }

    if(tp!=NULL){
        for(int i=0;i<3;i++){
            if(tp->bestMove==mv[i])
            {
                int bm=mv[i];
                for(int j=0;j<i;j++)
                    mv[j+1]=mv[j];
                mv[0]=bm;
                break;
            }
        }
    }

    int best = LOSE*2;
    for(int i=0;i<numMoves;i++){
        int m=mv[i];
        MoveUndo u=g.makeMove(p,m);
        int bestNext;
        int score = -negamax(g,other(p),depth-1,-beta,-alpha,bestNext);
        g.makeUndo(u);
        if(score>best){
            best=score;
            bestMove=m;
        }
        if(score>alpha){
            alpha=score;
            if(score>=beta){
                goto store_and_return;
            }
        }
    }

store_and_return:;
    TPTItem tpret;
    
    tpret.hash1=g.hash1();
    tpret.hash2=g.hash2();
    tpret.bestMove=bestMove;
    tpret.depth=depth;
    tpret.ply=g.ply();

    if(alpha==orig_alpha)
        tpret.type=UPPERBOUND;
    else if(alpha<beta)
        tpret.type=EXACT;
    else 
        tpret.type=LOWERBOUND;

    tpret.value=best;
    storeTPT(g,p,tpret);

    return alpha;
}


void dfstest(ColorGraph& g,int p,int d){

    if(d==0){
        total_pos++;
        return;
    }
    MoveList ml = g.getMoves();
    for(int i=0;i<3;i++){
        int m=ml[i];
        MoveUndo u = g.makeMove(p,m);
        dfstest(g,!p,d-1);
        g.makeUndo(u);
    }
}

int colorAiGetMove(ColorBoard* b, int player,int aiLevel){
    ColorGraph g(*b);
    int bm;
    int depth = aiLevel-2;
    int score = negamax(g,player,depth,2*LOSE,2*WIN,bm);
    score = negamax(g,player,depth+2,2*LOSE,2*WIN,bm);
    std::cout<<"score is "<<score<<"\n";
    return bm;
}



int main(int argc,char** argv){

    int bench=0;
    int size=32;
    for(int i=1;i<argc;i++){
        if(strcmp(argv[i],"-bench")==0){
            if(i+1>=argc){
                std::cout<<"-bench required parameter\n";
                return 1;
            }else{
                bench = atoi(argv[i+1]);
                i++;
            }
        }else {
            size=atoi(argv[i]);
            if(size<=0){
                std::cout<<"invalid size "<<argv[i]<<"\n";
                return 1;
            }
        }
    }


    initHashes();
        srand(time(0));
    ColorBoard b(size,size);

    std::cout<<b.getWidth()<<"x"<<b.getHeight()<<"\n";
    printBoard(b);

    if(bench!=0){
        while(b.getGameEnded()<0){
            int p = b.getTurn();
            std::cout<<"\nturn  "<<p<<"\n";
            int best=colorAiGetMove(&b,p,bench);
            b.makeMove(p,best);
            std::cout<<"best is "<<best<<"\n";
            printBoard(b);
            std::cout<<"positions visited: "<<total_pos<<"\n";
            std::cout<<"tp hits: "<<tp_hits<<"\n";
            std::cout<<"tp colls: "<<tp_cols<<"\n";
        }
        std::cout<<b.getGameEnded()<<" wins\n";
        return 0;
    }


    int turnNo=0;
    while(b.getGameEnded()<0){
        if(b.getTurn()==0){
            turnNo++;
            int humanPlay;
            MoveList allowed;
            int z=0;
            printBoard(b);

            std::cout<<"your turn #"<<turnNo<<"\n",
            std::cout<<"available moves:\n";
            for(int i=0;i<5;i++){
                if(i!=b.getPlayerColor(0)&&
                   i!=b.getPlayerColor(1)){
                    printColorBlock(i);
                    allowed[z++]=i;
                }else
                {
                    std::cout<<"  ";
                }
            }
            std::cout<<"\n";
            for(int i=0;i<5;i++){
                if(i!=b.getPlayerColor(0)&&
                   i!=b.getPlayerColor(1)){
                    std::cout<<i<<" ";
                    allowed[z++]=i;
                }else
                {
                    std::cout<<"  ";
                }
            }
            std::cout<<"\n";
            bool humanInput=true;
            while(humanInput){
                std::cin>>humanPlay;
                for(int i=0;i<3;i++){
                    if(allowed[i]==humanPlay)
                        humanInput=false;
                }
                if(humanInput){
                    std::cout<<"move "<<humanPlay<<" invalid, try again\n";
                }
            }
            b.makeMove(0,humanPlay);
        }else{
            int move = colorAiGetMove(&b,1,10);
            b.makeMove(1,move);
            std::cout<<"Bot plays "<<move<<" ";
            printColorBlock(move);
            std::cout<<"\n";
        }
        std::cout<<"positions visited: "<<total_pos<<"\n";
    }
    std::cout<<"winner is "<<b.getGameEnded()<<"\n";
}
