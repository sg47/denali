#ifndef DENALI_FOLDED_H
#define DENALI_FOLDED_H

#include <denali/graph_structures.h>
#include <denali/graph_mixins.h>
#include <denali/graph_maps.h>

namespace denali {

class FoldTree :
        public
        ReadableUndirectedGraphMixin <UndirectedGraph,
        BaseGraphMixin <UndirectedGraph> >
{
public:

    class NodeFold;
    class EdgeFold;

    typedef boost::shared_ptr<NodeFold> NodeFoldPtr;
    typedef boost::shared_ptr<EdgeFold> EdgeFoldPtr;

private:

    typedef
    ReadableUndirectedGraphMixin <UndirectedGraph,
    BaseGraphMixin <UndirectedGraph> >
    Mixin;

    typedef UndirectedGraph GraphType;

    GraphType _graph;

    ObservingNodeMap<GraphType, NodeFoldPtr> _node_to_fold;
    ObservingEdgeMap<GraphType, EdgeFoldPtr> _edge_to_fold;

    unsigned int _number_of_node_folds;

    Node restoreNode(NodeFoldPtr v_fold)
    {
        Node v = _graph.addNode();
        v_fold->_node = v;
        _node_to_fold[v] = v_fold;
        return v;
    }

    Edge restoreEdge(EdgeFoldPtr uv_fold)
    {
        Node u = uv_fold->_u_fold->_node;
        Node v = uv_fold->_v_fold->_node;

        Edge uv = _graph.addEdge(u,v);
        uv_fold->_edge = uv;
        _edge_to_fold[uv] = uv_fold;
        return uv;
    }

    NodeFoldPtr oppositeNodeFold(NodeFoldPtr u_fold, EdgeFoldPtr uv_fold)
    {
        return uv_fold->_u_fold->_id == u_fold->_id ? 
                uv_fold->_v_fold : uv_fold->_u_fold;
    }

public:
    typedef GraphType::Node Node;
    typedef GraphType::Edge Edge;

    class NodeFold
    {
        friend class FoldTree;

        unsigned int _id;
        std::vector<EdgeFoldPtr> _collapsed;
        EdgeFoldPtr _uv_fold;
        EdgeFoldPtr _vw_fold;
        Node _node;

        NodeFold(Node node, unsigned int id) : _node(node), _id(id) {}

    };

    class EdgeFold
    {
        friend class FoldTree;

        NodeFoldPtr _u_fold;
        NodeFoldPtr _v_fold;
        NodeFoldPtr _reduced_fold;
        Edge _edge;

        EdgeFold(Edge edge, NodeFoldPtr u_fold, NodeFoldPtr v_fold)
            : _edge(edge), _u_fold(u_fold), _v_fold(v_fold) {}

    };


    FoldTree() 
        : Mixin(_graph), _node_to_fold(_graph), _edge_to_fold(_graph),
          _number_of_node_folds(0) {}

    /// \brief Add a node to the tree.
    Node addNode()
    {
        // create the node
        Node node = _graph.addNode();

        // create the node fold
        NodeFoldPtr node_fold = NodeFoldPtr(new NodeFold(node, _number_of_node_folds));
        _number_of_node_folds++;

        // link the node to the node fold
        _node_to_fold[node] = node_fold;

        return node;
    }

    /// \brief Add an edge to the tree.
    Edge addEdge(Node u, Node v)
    {
        // create the edge
        Edge edge = _graph.addEdge(u,v);

        // get the node folds of u and v
        NodeFoldPtr u_fold = _node_to_fold[u];
        NodeFoldPtr v_fold = _node_to_fold[v];

        // make an edge fold
        EdgeFoldPtr edge_fold = EdgeFoldPtr(new EdgeFold(edge, u_fold, v_fold));

        // link it to the edge
        _edge_to_fold[edge] = edge_fold;

        return edge;
    }

    /// \brief Get the number of nodes that would exist in the fully unfolded tree.
    size_t numberOfNodeFolds() const
    {
        return _number_of_node_folds;
    }

    /// \brief Collapse the child node into the parent.
    void collapse(Edge edge)
    {
        Node parent = _graph.degree(_graph.u(edge)) == 1 ? 
                _graph.v(edge) : _graph.u(edge);

        Node child = _graph.opposite(parent, edge);

        assert(_graph.degree(child) == 1);

        // add the fold edge to the parent node fold's collapse list
        _node_to_fold[parent]->_collapsed.push_back(_edge_to_fold[edge]);

        // remove the child from the tree
        _graph.removeNode(child);
    }

    /// \brief Reduce the node, connecting its neighbors.
    Edge reduce(Node v)
    {
        // u <---> **v** <---> w
        assert(_graph.degree(v) == 2);

        // get the two neighbors
        UndirectedNeighborIterator<GraphType> it(_graph, v);
        Node u = it.neighbor(); Edge uv = it.edge(); ++it;
        Node w = it.neighbor(); Edge vw = it.edge();

        // make an edge between them
        Edge uw = this->addEdge(u,w);

        // update v's reduced edge folds
        NodeFoldPtr v_fold = _node_to_fold[v];
        v_fold->_uv_fold = _edge_to_fold[uv];
        v_fold->_vw_fold = _edge_to_fold[vw];

        // update the new edge's reduced node
        _edge_to_fold[uw]->_reduced_fold = v_fold;

        // remove the node
        _graph.removeNode(v);

        return uw;
    }

    /// \brief Uncollapses the collapsed edge at the index in the node's collapse list.
    /*!
     *  An index less than zero indicates that the last collapsed edge should be
     *  uncollapsed.
     */
    Edge uncollapse(Node u, int index=-1)
    {
        // get the folded node
        NodeFoldPtr u_fold = _node_to_fold[u];

        // if the index is less than one, uncollapse the last collapsed edge
        if (index < 0) {
            index = u_fold->_collapsed.size()-1;
        }

        // pop the collapsed edge fold
        EdgeFoldPtr uv_fold = u_fold->_collapsed[index];
        u_fold->_collapsed.erase(u_fold->_collapsed.begin() + index);

        // get the collapsed node fold at the other end of the edge
        NodeFoldPtr v_fold = oppositeNodeFold(u_fold, uv_fold);

        // restore the node
        Node v = restoreNode(v_fold);

        // restore the edge
        Edge uv = restoreEdge(uv_fold);

        return uv;
    }

    /// \brief Unreduces the edge.
    Node unreduce(Edge uw)
    {
        // u <---> **v** <---> w

        // get the reduced node
        EdgeFoldPtr uw_fold = _edge_to_fold[uw];
        NodeFoldPtr v_fold = uw_fold->_reduced_fold;

        // restore it to the tree
        Node v = restoreNode(v_fold);

        // get the reduced edges
        EdgeFoldPtr uv_fold = v_fold->_uv_fold;
        EdgeFoldPtr vw_fold = v_fold->_vw_fold;

        // restore the edges
        Edge uv = restoreEdge(uv_fold);
        Edge vw = restoreEdge(vw_fold);

        // remove the old edge
        _graph.removeEdge(uw);

        return v;
    }

};

}


#endif
