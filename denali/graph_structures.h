#ifndef DENALI_GRAPH_STRUCTURES_H
#define DENALI_GRAPH_STRUCTURES_H

#include <algorithm>
#include <list>
#include <vector>

#include <denali/graph_mixins.h>

namespace denali {

////////////////////////////////////////////////////////////////////////////
//
// VectorDirectedGraphImplementation
//
////////////////////////////////////////////////////////////////////////////

/// \brief This is the implementation class of VectorDirectedGraph.
/*!
 * It contains functionality that is not intended to be part of the
 * interface of VectorDirectedGraph.
 */
class VectorDirectedGraphImplementation
{
public:
    class Observer;

private:
    struct NodeRep
    {
        int first_in, first_out;
        int prev, next;
        bool valid;
        int in_degree;
        int out_degree;
    };

    struct ArcRep
    {
        int target, source;
        int prev_in, prev_out;
        int next_in, next_out;
        bool valid;
    };

    typedef std::vector<NodeRep> Nodes;
    Nodes nodes;
    int first_node;
    int first_free_node;

    typedef std::vector<ArcRep> Arcs;
    Arcs arcs;
    int first_free_arc;

    int number_of_nodes;
    int number_of_arcs;

    typedef std::list<Observer*> Observers;
    Observers _node_observers;
    Observers _arc_observers;

public:


    VectorDirectedGraphImplementation()
        : first_node(-1), first_free_node(-1), first_free_arc(-1),
          number_of_nodes(0), number_of_arcs(0) {};

    class Node
    {
        friend class VectorDirectedGraphImplementation;

    protected:
        int index;
        Node(int index) : index(index) {}

    public:
        Node() {}
        bool operator==(const Node& node) const {
            return index == node.index;
        }

        bool operator!=(const Node& node) const {
            return index != node.index;
        }

        bool operator<(const Node& node) const {
            return index < node.index;
        }
    };


    class Arc
    {
        friend class VectorDirectedGraphImplementation;

    protected:
        int index;
        Arc(int index) : index(index) {}

    public:
        Arc() {}
        bool operator==(const Arc& arc) const {
            return index == arc.index;
        }

        bool operator!=(const Arc& arc) const {
            return index != arc.index;
        }

        bool operator<(const Arc& arc) const {
            return index < arc.index;
        }
    };


    class Observer
    {
    public:
        virtual void notify() = 0;
        virtual ~Observer() {}
    };


    void notifyNodeObservers() const
    {
        for (Observers::const_iterator it = _node_observers.begin();
                it != _node_observers.end();
                ++it) {
            (*it)->notify();
        }
    }

    void notifyArcObservers() const
    {
        for (Observers::const_iterator it = _arc_observers.begin();
                it != _arc_observers.end();
                ++it) {
            (*it)->notify();
        }
    }

    Node addNode()
    {
        // the index of the node in the vector
        int n;

        // if there isn't a free node, we push a new one
        if (first_free_node == -1) {
            n = nodes.size();
            nodes.push_back(NodeRep());
        } else {
            n = first_free_node;
            // the new free node was pointed to by the old first free
            first_free_node = nodes[n].next;
        }

        nodes[n].next = first_node;

        // if the old first node was valid, we update it's links
        if (first_node != -1) {
            nodes[first_node].prev = n;
        }

        first_node = n;
        nodes[n].prev = -1;

        // the new node has no arcs
        nodes[n].first_in = nodes[n].first_out = -1;

        // the node is valid
        nodes[n].valid = true;

        // the degrees of the node need to be set to zero
        nodes[n].in_degree = nodes[n].out_degree = 0;

        // one more node
        number_of_nodes++;

        // notify
        notifyNodeObservers();

        return Node(n);
    }

    Arc addArc(const Node u, const Node v)
    {
        // the index of the arc in the vector
        int n;

        // check to see if there is an available slot
        if (first_free_arc == -1) {
            n = arcs.size();
            arcs.push_back(ArcRep());
        } else {
            n = first_free_arc;
            first_free_arc = arcs[n].next_in;
        }

        // assign source and targets of the edge
        arcs[n].source = u.index;
        arcs[n].target = v.index;

        // make this the first out of the source node, and make the
        // existing first out's prev this node
        arcs[n].next_out = nodes[u.index].first_out;
        if (nodes[u.index].first_out != -1) {
            arcs[nodes[u.index].first_out].prev_out = n;
        }

        // make this the first in of the target node, and make the
        // existing first in's prev this node
        arcs[n].next_in = nodes[v.index].first_in;
        if (nodes[v.index].first_in != -1) {
            arcs[nodes[v.index].first_in].prev_in = n;
        }

        // this is the first in and out arc: make the prev in and out
        // invalid
        arcs[n].prev_in = arcs[n].prev_out = -1;

        nodes[u.index].first_out = nodes[v.index].first_in = n;

        // the arc is valid
        arcs[n].valid = true;

        // the source's out degree is +1
        nodes[u.index].out_degree++;

        // the target's in degree is +1
        nodes[v.index].in_degree++;

        // one more arc
        number_of_arcs++;

        // notify
        notifyArcObservers();

        return Arc(n);
    }

    int numberOfNodes() const {
        return number_of_nodes;
    }
    int numberOfArcs() const {
        return number_of_arcs;
    }

    void eraseNode(const Node node)
    {
        // This is a helper function to remove a node from the representation.
        // Using it without doing the proper actions before and after erasing
        // the node may leave the graph in an invalid state.
        //
        // Namely, erasing a node does nothing to remove the arcs attached to
        // the node: they will be left hanging.
        //
        // eraseNode does:
        //      - make the node linked list consistent
        //      - marks the node as free
        //      - marks the node as invalid
        //      - decrements the number of nodes counter

        int n = node.index;

        // if the next and prev nodes are valid connect them

        if (nodes[n].next != -1) {
            nodes[nodes[n].next].prev = nodes[n].prev;
        }

        if (nodes[n].prev != -1) {
            nodes[nodes[n].prev].next = nodes[n].next;
        } else {
            // this must have been the first node. Not anymore.
            first_node = nodes[n].next;
        }

        nodes[n].next = first_free_node;
        first_free_node = n;

        nodes[n].valid = false;

        number_of_nodes--;
    }

    void eraseArc(const Arc arc)
    {
        // This is a helper function to remove an arc from the representation.
        // Using it without doing the proper actions before and after erasing
        // the arc may leave the graph in an invalid state.
        //
        // eraseArc does:
        //      - make the arc linked list consistent
        //      - marks the arc as free
        //      - marks the arc as invalid
        //      - decrements the number of arcs counter
        //      - decrements the degrees of associated nodes

        int n = arc.index;

        if (arcs[n].next_in != -1) {
            arcs[arcs[n].next_in].prev_in = arcs[n].prev_in;
        }

        if (arcs[n].prev_in != -1) {
            arcs[arcs[n].prev_in].next_in = arcs[n].next_in;
        } else {
            nodes[arcs[n].target].first_in = arcs[n].next_in;
        }

        if (arcs[n].next_out != -1) {
            arcs[arcs[n].next_out].prev_out = arcs[n].prev_out;
        }

        if (arcs[n].prev_out != -1) {
            arcs[arcs[n].prev_out].next_out = arcs[n].next_out;
        } else {
            nodes[arcs[n].source].first_out = arcs[n].next_out;
        }

        arcs[n].next_in = first_free_arc;
        first_free_arc = n;

        arcs[n].valid = false;

        // the source node's out degree is -1
        nodes[arcs[n].source].out_degree--;

        // the target node's in degree is -1
        nodes[arcs[n].target].in_degree--;

        number_of_arcs--;
    }


    void removeNode(const Node node)
    {
        // This is the high-level method to remove a node. It should
        // be called by the handle, not eraseNode.
        //
        // eraseNode does:
        //      - remove all arcs associated with the node
        //      - notify on node removal

        Arc arc;

        // delete all of the out arcs
        arc = firstOutArc(node);
        while (isArcValid(arc)) {
            removeArc(arc);
            arc = firstOutArc(node);
        }

        // delete all of the in arcs
        arc = firstInArc(node);
        while (isArcValid(arc)) {
            removeArc(arc);
            arc = firstInArc(node);
        }

        // notify
        notifyNodeObservers();

        // finally, remove the node
        eraseNode(node);
    }

    void removeArc(const Arc arc)
    {
        // This is the high-level method to remove an arc. It should
        // be called by the handle, not eraseArc.
        //
        // removeArc does:
        //      - notify on arc removal

        notifyArcObservers();

        eraseArc(arc);
    }

    int degree(const Node node) const
    {
        return nodes[node.index].in_degree + nodes[node.index].out_degree;
    }

    int inDegree(const Node node) const
    {
        return nodes[node.index].in_degree;
    }

    int outDegree(const Node node) const
    {
        return nodes[node.index].out_degree;
    }

    Arc firstOutArc(const Node node) const
    {
        // Returns the first out arc of the node.
        return Arc(nodes[node.index].first_out);
    }

    Arc firstInArc(const Node node) const
    {
        // Returns the first in arc of the node.
        return Arc(nodes[node.index].first_in);
    }

    bool isNodeValid(const Node node) const
    {
        return node.index >= 0 && node.index < nodes.size() &&
               nodes[node.index].valid;
    }

    bool isArcValid(const Arc arc) const
    {
        return arc.index >= 0 && arc.index < arcs.size() &&
               arcs[arc.index].valid;
    }

    Node getFirstNode() const
    {
        return Node(first_node);
    }

    Node getNextNode(Node node) const
    {
        return nodes[node.index].next;
    }

    Node source(const Arc arc) const
    {
        return Node(arcs[arc.index].source);
    }

    Node target(const Arc arc) const
    {
        return Node(arcs[arc.index].target);
    }

    Node opposite(const Node node, const Arc arc) const
    {
        Node source_node = source(arc);
        return node == source_node ? target(arc) : source_node;
    }

    Arc getFirstArc() const
    {
        // we iterate through the nodes until we find one with an out arc
        // that isn't invalid
        Node node = getFirstNode();
        while (isNodeValid(node) && nodes[node.index].first_out == -1) {
            node = getNextNode(node);
        }

        if (!isNodeValid(node)) {
            return Arc(-1);
        } else {
            return Arc(nodes[node.index].first_out);
        }
    }

    Arc getNextArc(Arc arc) const
    {
        if (arcs[arc.index].next_out != -1) {
            return Arc(arcs[arc.index].next_out);
        } else {
            Node node = nodes[source(arc).index].next;
            while (isNodeValid(node) && nodes[node.index].first_out == -1) {
                node = getNextNode(node);
            }

            if (!isNodeValid(node)) {
                return Arc(-1);
            } else {
                return Arc(nodes[node.index].first_out);
            }
        }
    }

    Arc getFirstOutArc(const Node node) const
    {
        return Arc(nodes[node.index].first_out);
    }

    Arc getNextOutArc(const Arc arc) const
    {
        return Arc(arcs[arc.index].next_out);
    }

    Arc getFirstInArc(const Node node) const
    {
        return Arc(nodes[node.index].first_in);
    }

    Arc getNextInArc(const Arc arc) const
    {
        return Arc(arcs[arc.index].next_in);
    }

    Arc getFirstNeighborArc(const Node node) const
    {
        Arc arc = getFirstOutArc(node);
        if (isArcValid(arc)) {
            return arc;
        } else {
            return getFirstInArc(node);
        }
    }

    Arc getNextNeighborArc(const Node node, const Arc arc) const
    {
        if (source(arc) == node) {
            Arc next_arc = getNextOutArc(arc);
            if (!isArcValid(next_arc)) {
                return getFirstInArc(node);
            }
            return next_arc;
        } else {
            return getNextInArc(arc);
        }
    }

    unsigned int getNodeIdentifier(const Node node) const
    {
        return node.index;
    }

    unsigned int getArcIdentifier(const Arc arc) const
    {
        return arc.index;
    }

    unsigned int getMaxNodeIdentifier() const
    {
        return nodes.size() ;
    }

    unsigned int getMaxArcIdentifier() const
    {
        return arcs.size();
    }

    void attachNodeObserver(Observer& ob)
    {
        _node_observers.push_back(&ob);
    }

    void detachNodeObserver(Observer& ob)
    {
        _node_observers.remove(&ob);
    }

    void attachArcObserver(Observer& ob)
    {
        _arc_observers.push_back(&ob);
    }

    void detachArcObserver(Observer& ob)
    {
        _arc_observers.remove(&ob);
    }

    Arc findArc(const Node source_node, const Node target_node) const
    {
        // we iterate through the out arcs of the source, and check
        // the target. If we make it all the way through, we return
        // the invalid arc.
        Arc arc = getFirstOutArc(source_node);
        while (isArcValid(arc)) {
            if (target(arc) == target_node) return arc;
            arc = getNextOutArc(arc);
        }

        // if we haven't found anything yet, return the invalid arc
        return arc;
    }

    Node getInvalidNode() const
    {
        return Node(-1);
    }

    Arc getInvalidArc() const
    {
        return Arc(-1);
    }

    void clearNodes()
    {
        for (int i=0; i<nodes.size(); ++i) {
            if (isNodeValid(Node(i))) {
                removeNode(Node(i));
            }
        }
    }

    void clearArcs()
    {
        for (int i=0; i<arcs.size(); ++i) {
            if (isArcValid(Arc(i))) {
                removeArc(Arc(i));
            }
        }
    }

};


////////////////////////////////////////////////////////////////////////////
//
// DirectedGraph
//
////////////////////////////////////////////////////////////////////////////

/// \brief The base of a directed graph class.
/*!
 *  Implementation must conform to:
 *   - NodeObservable
 *   - ArcObservable
 *   - WritableReadableDirectedGraph
 */
template <typename Implementation>
class DirectedGraphBase :
    public
    NodeObservableMixin <Implementation,
    ArcObservableMixin <Implementation,
    WritableReadableDirectedGraphMixin <Implementation,
    BaseGraphMixin <Implementation> > > >
{
    typedef
    NodeObservableMixin <Implementation,
                        ArcObservableMixin <Implementation,
                        WritableReadableDirectedGraphMixin <Implementation,
                        BaseGraphMixin <Implementation> > > >
                        Mixin;

    Implementation _graph;

public:
    DirectedGraphBase() : Mixin(_graph) {}
};


/// \brief The principal directed graph class.
/// \ingroup graph_implementations_structures
/*!
 *  This is a concrete implementation of
 *   - concepts::WritableReadableDirectedGraph
 *   - concepts::NodeObservable
 *   - concepts::ArcObservable
 *
 *  It's advised that you use the concept class documentation page as a
 *  reference instead of this page.
 */
class DirectedGraph :
    public DirectedGraphBase<VectorDirectedGraphImplementation>
{
    typedef
    DirectedGraphBase<VectorDirectedGraphImplementation>
    Base;

public:
    typedef Base::Node Node;
    typedef Base::Arc Arc;
    typedef Base::Observer Observer;
};


////////////////////////////////////////////////////////////////////////////
//
// UndirectedGraphImplementation
//
////////////////////////////////////////////////////////////////////////////

/// \brief An implementation of an undirected graph.
/*!
 *  This implementation uses an implentation (GraphType) of the
 *  concepts::WritableReadableDirectedGraph concept as its base. The base
 *  must also fulfill concepts::NodeObservable and concepts::EdgeObservable.
 *
 *  It fulfills
 *  the concepts::WritableReadableUndirectedGraph concept, as well as
 *  the concepts::NodeObservable and concepts::EdgeObservable concepts.
 */
template <typename GraphType>
class UndirectedGraphImplementation
{
    typedef GraphType Impl;

    Impl impl;

public:
    class Node
    {
        friend class UndirectedGraphImplementation;
        typename Impl::Node base;
        Node(typename Impl::Node _base) {
            base = _base;
        }

    public:
        Node() {}

        bool operator==(const Node& node) const {
            return base == node.base;
        }

        bool operator!=(const Node& node) const {
            return base != node.base;
        }

        bool operator<(const Node& node) const {
            return base < node.base;
        }
    };

    class Edge
    {
        friend class UndirectedGraphImplementation;
        typename Impl::Arc base;
        Edge(typename Impl::Arc _base) {
            base = _base;
        }

    public:
        Edge() {}

        bool operator==(const Edge& edge) const {
            return base == edge.base;
        }

        bool operator!=(const Edge& edge) const {
            return base != edge.base;
        }

        bool operator<(const Edge& edge) const {
            return base < edge.base;
        }
    };

    typedef typename GraphType::Observer Observer;


    bool isNodeValid(Node node) const {
        return impl.isNodeValid(node.base);
    }
    bool isEdgeValid(Edge edge) const {
        return impl.isArcValid(edge.base);
    }
    Node getFirstNode() const {
        return Node(impl.getFirstNode());
    }
    Node getNextNode(Node node) const {
        return Node(impl.getNextNode(node.base));
    }
    Edge getFirstEdge() const {
        return Edge(impl.getFirstArc());
    }
    Edge getNextEdge(Edge edge) const {
        return Edge(impl.getNextArc(edge.base));
    }

    Edge getFirstNeighborEdge(Node node) const
    {
        return Edge(impl.getFirstNeighborArc(node.base));
    }

    Edge getNextNeighborEdge(Node node, Edge edge) const
    {
        return Edge(impl.getNextNeighborArc(node.base, edge.base));
    }

    Node opposite(Node node, Edge edge) const
    {
        return Node(impl.opposite(node.base, edge.base));
    }

    unsigned int degree(Node node) const {
        return impl.degree(node.base);
    }
    Node u(Edge edge) const {
        return impl.source(edge.base);
    }
    Node v(Edge edge) const {
        return impl.target(edge.base);
    }

    unsigned int numberOfNodes() const {
        return impl.numberOfNodes();
    }
    unsigned int numberOfEdges() const {
        return impl.numberOfArcs();
    }

    unsigned int getMaxNodeIdentifier() const
    {
        return impl.getMaxNodeIdentifier();
    }
    unsigned int getNodeIdentifier(Node node) const
    {
        return impl.getNodeIdentifier(node.base);
    }

    unsigned int getMaxEdgeIdentifier() const
    {
        return impl.getMaxArcIdentifier();
    }
    unsigned int getEdgeIdentifier(Edge edge) const
    {
        return impl.getArcIdentifier(edge.base);
    }

    Node addNode() {
        return Node(impl.addNode());
    }
    Edge addEdge(Node u, Node v)
    {
        return Edge(impl.addArc(u.base, v.base));
    }

    void removeNode(Node node) {
        impl.removeNode(node.base);
    }
    void removeEdge(Edge edge) {
        impl.removeArc(edge.base);
    }

    void clearNodes() {
        impl.clearNodes();
    }
    void clearEdges() {
        impl.clearArcs();
    }

    void attachNodeObserver(Observer& ob) {
        impl.attachNodeObserver(ob);
    }
    void detachNodeObserver(Observer& ob) {
        impl.detachNodeObserver(ob);
    }
    void attachEdgeObserver(Observer& ob) {
        impl.attachArcObserver(ob);
    }
    void detachEdgeObserver(Observer& ob) {
        impl.detachArcObserver(ob);
    }

    Edge findEdge(Node u, Node v) const
    {
        // We can gain some performance by searching the node with
        // the smaller out degree first.

        typename Impl::Node smaller;
        typename Impl::Node bigger;

        if (impl.outDegree(u.base) < impl.outDegree(v.base)) {
            smaller = u.base;
            bigger = v.base;
        } else {
            smaller = v.base;
            bigger = u.base;
        }

        typename Impl::Arc arc = impl.findArc(smaller, bigger);
        if (impl.isArcValid(arc)) {
            return Edge(arc);
        } else {
            return Edge(impl.findArc(bigger, smaller));
        }
    }

    Node getInvalidNode() const {
        return impl.getInvalidNode();
    }
    Edge getInvalidEdge() const {
        return Edge(impl.getInvalidArc());
    }

};

////////////////////////////////////////////////////////////////////////////
//
// UndirectedGraph
//
////////////////////////////////////////////////////////////////////////////

/// \brief A base class for an undirected graph.
/*!
 *  Implementation must conform to:
 *   - EdgeObservable
 *   - NodeObservable
 *   - WritableReadableUndirectedGraphMixin
 *
 */
template <typename Implementation>
class UndirectedGraphBase :
    public
    EdgeObservableMixin <Implementation,
    NodeObservableMixin <Implementation,
    WritableReadableUndirectedGraphMixin <Implementation,
    BaseGraphMixin <Implementation> > > >
{
    typedef
    EdgeObservableMixin <Implementation,
                        NodeObservableMixin <Implementation,
                        WritableReadableUndirectedGraphMixin <Implementation,
                        BaseGraphMixin <Implementation> > > >
                        Mixin;

    Implementation _graph;

public:
    UndirectedGraphBase() : Mixin(_graph) {}

};

/// \brief The principal undirected graph class.
/// \ingroup graph_implementations_structures
/*!
 *  This is a concrete implementation of
 *   - concepts::WritableReadableUndirectedGraph
 *   - concepts::NodeObservable
 *   - concepts::EdgeObservable
 *
 *  It's advised that you use the concept class documentation page as a
 *  reference instead of this page.
 */
class UndirectedGraph :
    public
    UndirectedGraphBase <
    UndirectedGraphImplementation <
    DirectedGraph > >
{
    typedef
    UndirectedGraphBase <
    UndirectedGraphImplementation <
    DirectedGraph > >
    Base;

public:
    typedef Base::Node Node;
    typedef Base::Edge Edge;
    typedef Base::Observer Observer;

};




}

#endif
