//
// Created by wyz on 2021/5/13.
//

#ifndef NEURONANNOTATION_AUTOPATHFIND_HPP
#define NEURONANNOTATION_AUTOPATHFIND_HPP
#include <vector>
#include <array>
#include <Image.hpp>
#include <seria/object.hpp>
#include <stack>
#include <Common/utils.hpp>
#include <algorithm>
#include <unordered_set>
/**
 * using dijkstra algorithm
 */
class AutoPathGen{
public:
    AutoPathGen()=default;
    std::array<uint32_t,2> point0;
    std::array<uint32_t,2> point1;

    enum class NodeState{
        EXPANDED,
        INITIAL,ACTIVE
    };
    struct Node{
        std::vector<std::pair<Node*,float>> near_nodes;
        Node* prev;
        NodeState state;
        int row,col;
        float cost;//cost from seed to current node
        bool operator<(const Node& node) const{
            return cost>node.cost;
        }
        bool operator<(const Node* node) const{
            return cost>node->cost;
        }
    };
    //ref https://www.cs.cornell.edu/courses/cs4670/2012fa/projects/p1_iscissors/project1.html#to_do
    std::vector<std::array<float,4>> GenPath_v1(const Map<float>& map){
        point0[0]=map.height-point0[0];
        point1[0]=map.height-point1[0];
        auto start_p = map.at(point0[0],point0[1]);
        auto stop_p  = map.at(point1[0],point1[1]);
        print_array(point0);
        print_array(point1);
        std::cout<<"start_p: "<<start_p[0]<<" "<<start_p[1]<<" "<<start_p[2]<<" "<<start_p[3]<<std::endl;
        std::cout<<"stop_p: "<<stop_p[0]<<" "<<stop_p[1]<<" "<<stop_p[2]<<" "<<stop_p[3]<<std::endl;
        std::cout<<__FUNCTION__ <<std::endl;

        auto isValid=[map](int row,int col)->bool{
            if(row>=0 && row<map.height && col>=0 && col<map.width){
                return true;
            }
            else
                return false;
        };
        auto distance=[](const std::array<float,4>& p0,const std::array<float,4>& p1)->float{
            return std::sqrt((p0[0]-p1[0])*(p0[0]-p1[0])+(p0[1]-p1[1])*(p0[1]-p1[1])+(p0[2]-p1[2])*(p0[2]-p1[2]));
        };
        constexpr float inf=65536.f;
        std::vector<std::vector<Node>> nodes;
        nodes.resize(map.height);
        for(int row=0;row<map.height;row++){
            nodes[row].resize(map.width);
        }
        for(int row=0;row<map.height;row++){
            for(int col=0;col<map.width;col++){
                nodes[row][col].state=NodeState::INITIAL;
                nodes[row][col].cost=inf;
                nodes[row][col].prev=nullptr;
                nodes[row][col].row=row;
                nodes[row][col].col=col;
                nodes[row][col].near_nodes.reserve(8);
                auto node_pos=map.at(row,col);
                for(int row_offset=-1;row_offset<2;row_offset++){
                    for(int col_offset=-1;col_offset<2;col_offset++){
                        if(row_offset || col_offset){
                            if(isValid(row+row_offset,col+col_offset)){
                                auto near_node=map.at(row+row_offset,col+col_offset);
                                nodes[row][col].near_nodes.emplace_back(std::make_pair(&nodes[row+row_offset][col+col_offset],distance(node_pos,near_node)));
                            }
                            else{
                                nodes[row][col].near_nodes.emplace_back(std::make_pair(nullptr,inf));
                            }
                        }
                    }
                }
            }
        }
        struct cmp{
            bool operator()(const Node* a,const Node* b)const{
                return a->cost>b->cost;
            }
        };
        std::priority_queue<Node*,std::vector<Node*>,cmp> pq;
        auto seed=&nodes[point0[0]][point0[1]];
        seed->cost=0.f;
        pq.push(seed);
        while(!pq.empty()){
            auto q=pq.top();
            pq.pop();
            q->state=NodeState::EXPANDED;
            for(auto& neighbor:q->near_nodes){
                if(neighbor.first){
                    if(neighbor.first->state==NodeState::INITIAL){
                        neighbor.first->state=NodeState::ACTIVE;
                        neighbor.first->cost=q->cost+neighbor.second;
                        neighbor.first->prev=q;
                        pq.push(neighbor.first);
                    }
                    else if(neighbor.first->state==NodeState::ACTIVE){
                        if(q->cost+neighbor.second<neighbor.first->cost){
                            neighbor.first->cost=q->cost+neighbor.second;
                            neighbor.first->prev=q;
                        }
                    }
                }
            }
        }
        auto target=nodes[point1[0]][point1[1]];
        std::cout<<"seed to  target cost: "<<target.cost<<std::endl;

        std::vector<std::array<float,4>> path;
        for(Node* p=&target;p!=seed;p=p->prev){
            if(!p){
                std::cout<<"p is nullptr"<<std::endl;
                break;
            }
            path.push_back(map.at(p->row,p->col));

        }
        std::cout<<"finish path"<<std::endl;
        return path;
    }
    std::vector<std::array<float,4>> GenPath(const Map<float>& map){
        float step=1.f;
        float alpha=0.6f;

        std::vector<bool> visit;
        std::vector<float> dist;
        std::vector<std::array<float,9>> edges;
        std::vector<int> prev;
        point0[1]=map.height-point0[1];
        point1[1]=map.height-point1[1];

        auto start_p = map.at(point0[0],point0[1]);
        auto stop_p  = map.at(point1[0],point1[1]);
        if(start_p[3]<alpha || stop_p[3]<alpha){
            std::cout<<"start_p: "<<start_p[0]<<" "<<start_p[1]<<" "<<start_p[2]<<" "<<start_p[3]<<std::endl;
            std::cout<<"stop_p: "<<stop_p[0]<<" "<<stop_p[1]<<" "<<stop_p[2]<<" "<<stop_p[3]<<std::endl;
            std::cout<<"start or stop point invalid."<<std::endl;
            return {};
        }
        size_t node_num=(size_t)map.height*map.width;
        visit.assign(node_num,false);
        dist.assign(node_num,65536.f);
        prev.assign(node_num,-1);

        auto distance=[](const std::array<float,4>& p0,const std::array<float,4>& p1)->float{
            return std::sqrt((p0[0]-p1[0])*(p0[0]-p1[0])+(p0[1]-p1[1])*(p0[1]-p1[1])+(p0[2]-p1[2])*(p0[2]-p1[2]));
        };
        auto toflatidx=[map](int row,int col)->size_t{
            return (size_t)row*map.width+col;
        };
        auto isValid=[map](int row,int col)->bool{
            if(row>=0 && row<map.height && col>=0 && col<map.width){
                return true;
            }
            else
                return false;
        };
        for(int i=0;i<map.height;i++){
            for(int j=0;j<map.width;j++){
                if(i>0 && i<map.height-1 && j>0 && j<map.width-1){
                    std::array<float,4> center_pos=map.at(i,j);
                    std::array<float,9> node_dist{};
                    for(int row_offset=-1;row_offset<2;row_offset++){
                        for(int col_offset=-1;col_offset<2;col_offset++){
                            std::array<float,4> near_node=map.at(i+row_offset,j+col_offset);
                            node_dist[(row_offset+1)*3+col_offset+1]=distance(center_pos,near_node);
                        }
                    }
                    node_dist[4]=65536.f;//center_pos self
                    edges.push_back(node_dist);
                }
            }
        }
        auto target_idx=toflatidx(point1[0],point1[1]);
        dist[toflatidx(point0[0],point0[1])]=0.f;
        std::unordered_set<size_t> near_nodes;
        near_nodes.insert(toflatidx(point0[0],point0[1]));
        for(size_t i=0;i<node_num;i++){
            std::cout<<"i: "<<i<<std::endl;

            int u=-1;
            float temp_min_dist=65536.f;

//            for(int j=0;j<node_num;j++){
//                if(!visit[j] && dist[j] < temp_min_dist){
//                    u=j;
//                    temp_min_dist=dist[j];
//                }
//            }
            START_CPU_TIMER
            for(auto it:near_nodes){
                if(dist[it]<temp_min_dist){
                    u=it;
                    temp_min_dist=dist[it];
                }
            }
            END_CPU_TIMER
            near_nodes.erase(u);

            if(u==-1){
                std::cout<<"u==-1 and break"<<std::endl;
                break;
            }

            visit[u]=true;

            int row=u/map.width;
            int col=u-row*map.width;

            for(int v=0;v<9;v++){//for near node dist update
                int row_offset=v/3;
                int col_offset=v-row_offset*3;
                row_offset--;
                col_offset--;
                int v_row=row+row_offset;
                int v_col=col+col_offset;
                size_t v_flat_idx=toflatidx(v_row, v_col);
                if(isValid(v_row,v_col)){
                    if(!visit[v_flat_idx] && edges[u][v] != 65536.f){
                        if(dist[v_flat_idx]>dist[u]+edges[u][v]){
                            dist[v_flat_idx]=dist[u]+edges[u][v];
                            prev[v_flat_idx]=u;

                        }
                        if(dist[v_flat_idx]<65536.f && !visit[v_flat_idx]){
                            near_nodes.insert(v_flat_idx);
                        }
                    }
                }
            }
            if(u==target_idx){
                break;
            }


        }

        std::cout<<dist[target_idx]<<std::endl;

    }
};
namespace seria{
    template<>
    inline auto register_object<AutoPathGen>(){
                return std::make_tuple(
                member("point0",&AutoPathGen::point0),
                member("point1",&AutoPathGen::point1));
    }
}

#endif //NEURONANNOTATION_AUTOPATHFIND_HPP
