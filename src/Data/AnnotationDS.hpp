//
// Created by wyz on 2021/5/3.
//

#ifndef NEURONANNOTATION_ANNOTATIONDS_HPP
#define NEURONANNOTATION_ANNOTATIONDS_HPP



#include<vector>
#include<list>
#include<memory>
#include<map>
#include<glm/glm.hpp>
class NeuronGraph;
struct Vertex{
    glm::vec3 pos;
    int vertex_index;
    NeuronGraph* graph;
};
struct Edge{
    Vertex *v0,*v1;
    NeuronGraph* graph;
    int edge_index;
    float length;
};
struct Line{
    std::list<Edge*> edges;
    int line_index;
    NeuronGraph* graph;
};
class NeuronGraph{
public:
    explicit NeuronGraph(int idx):graph_index(idx){}

    bool selectVertices(std::vector<int> idxes);
    bool selectEdges(std::vector<int> idxes);
    bool selectLines(std::vector<int> idxes);
    bool deleteCurSelectVertices();
    bool deleteCurSelectEdges();
    bool deleteCurSelectLines();
    bool addVertex(Vertex* v);
private:
    int graph_index;
    std::map<int,Edge*> edges;
    std::map<int,Vertex*> vertices;
    std::map<int,Line*> lines;
    std::vector<Vertex*> cur_select_vertices;//last add or current pick
    std::vector<Edge*> cur_select_edges;//last add edge or current select edge
    std::vector<Line*> cur_select_lines;
    int select_obj;//0 for nothing, 1 for point, 2 for edge, 3 for points, 4 for edges,5 for line,6 for lines
};
class NeuronGraphDB{
public:
    NeuronGraphDB()=default;
private:
    bool isValid(int user_id) const{
        return user_id==owner_id;
    }
private:
    std::vector<std::unique_ptr<NeuronGraph>> graphs;
    int owner_id;
};

class NeuronPool{
public:
    bool addVertex(Vertex v);

private:
    std::shared_ptr<NeuronGraph> cur_edit_graph;
    Vertex last_add_v;
    Edge last_add_e;
private:
    int user_id;
    std::vector<std::unique_ptr<NeuronGraphDB>> graphs;
};


#endif //NEURONANNOTATION_ANNOTATIONDS_HPP
