// Copyright (c) 2014, Justin Eldridge, Mikhail Belkin, and Yusu Wang
// at The Ohio State University. All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef DENALI_RECTANGULAR_LANDSCAPE_H
#define DENALI_RECTANGULAR_LANDSCAPE_H

#include <denali/graph_iterators.h>
#include <denali/graph_maps.h>
#include <denali/graph_mixins.h>
#include <denali/landscape.h>
#include <denali/rectangular_landscape.h>

#include <boost/shared_ptr.hpp>

#include <cmath>
#include <stack>
#include <stdexcept>
#include <vector>

namespace denali {
namespace rectangular {

class Rectangle;
class RectangleSplitter;

class RectangleSplit;
class OrientedRectangleSplit;
class HorizontalRectangleSplit;
class VerticalRectangleSplit;

template <typename LandscapeTree> class Embedding;
template <typename LandscapeTree> class Embedder;
template <typename LandscapeTree> struct EmbeddingParameters;

template <typename LandscapeTree> class Triangularization;
template <typename LandscapeTree> class Triangularizer;

}

template <typename ContourTree> class RectangularLandscape;
template <typename ContourTree> class RectangularLandscapeBuilder;
}

////////////////////////////////////////////////////////////////////////
//
// Rectangle
//
////////////////////////////////////////////////////////////////////////

class denali::rectangular::Rectangle
{
    double x;
    double y;
    double width_;
    double height_;

public:
    class Point
    {
        double x_;
        double y_;
    public:
        Point(double x, double y) : x_(x), y_(y) {}
        double x() const {
            return x_;
        }
        double y() const {
            return y_;
        }
    };

    Rectangle(double x, double y, double width, double height)
        : x(x), y(y), width_(width), height_(height)
    {
        if (width <= 0 || height <= 0)
            throw std::invalid_argument("Width and height must be positive.");
    }

    double area() const {
        return width_*height_;
    }
    double width() const {
        return width_;
    }
    double height() const {
        return height_;
    }
    Point center() const {
        return Point(x,y);
    }
    Point sw() const {
        return Point(x-width_/2, y-height_/2);
    }
    Point se() const {
        return Point(x+width_/2, y-height_/2);
    }
    Point ne() const {
        return Point(x+width_/2, y+height_/2);
    }
    Point nw() const {
        return Point(x-width_/2, y+height_/2);
    }
    Rectangle shrink(const double) const;
    std::ostream& print(std::ostream& out) const;
};


denali::rectangular::Rectangle
denali::rectangular::Rectangle::shrink(
    const double factor) const
{
    if (factor <= 0 || factor > 1)
        throw std::invalid_argument("Shrink factor must be 0 < factor <= 1.");

    double lambda = std::sqrt(factor);
    return Rectangle(x, y, lambda * width_, lambda * height_);
}


std::ostream&
denali::rectangular::Rectangle::print(
    std::ostream& out) const
{
    out << "Rectangle(sw=" << "(" << sw().x() << "," << sw().y() << "), "
        << "se=(" << se().x() << "," << se().y() << "), "
        << "ne=(" << ne().x() << "," << ne().y() << "), "
        << "nw=(" << nw().x() << "," << nw().y() << "))";
    return out;
}


std::ostream& operator<<(
    std::ostream& out,
    const denali::rectangular::Rectangle& rectangle)
{
    return rectangle.print(out);
}

////////////////////////////////////////////////////////////////////////
//
// RectangleSplitter
//
////////////////////////////////////////////////////////////////////////




class denali::rectangular::OrientedRectangleSplit
{
protected:
    std::vector<Rectangle> rectangles;

public:
    virtual ~OrientedRectangleSplit() { }

    void addRectangle(Rectangle rect) {
        rectangles.push_back(rect);
    }
    Rectangle getRectangle(size_t i) const {
        return rectangles[i];
    }
    size_t numberOfRectangles() const {
        return rectangles.size();
    }

    Rectangle::Point operator[](size_t i) const {
        return getBoundaryPoint(i);
    }
    size_t size() const {
        return numberOfRectangles()*2 + 2;
    }

    virtual Rectangle::Point getBoundaryPoint(size_t id) const = 0;
    virtual size_t getIndexOfCorner(size_t i) const = 0;
    virtual size_t getIndexOfRectangleCorner(size_t rectangle, size_t corner) const = 0;
};


class denali::rectangular::HorizontalRectangleSplit : public OrientedRectangleSplit
{
public:
    virtual Rectangle::Point getBoundaryPoint(size_t i) const
    {
        unsigned int n = rectangles.size();

        if (i == 0) return rectangles[0].sw();
        if (i == n + 1) return rectangles[n-1].ne();
        if (i < n + 1) return rectangles[i-1].se();

        // i > n+1
        return rectangles[(2*n+1) - i].nw();
    }

    virtual size_t getIndexOfCorner(size_t i) const
    {
        unsigned int n = rectangles.size();

        if (i == 0) return 0;
        if (i == 1) return 1;
        if (i == 2) return n+1;

        // i == 3
        return n+2;
    }

    size_t getIndexOfRectangleCorner(size_t rectangle, size_t corner) const
    {
        unsigned int n = rectangles.size();

        if (corner == 0) return (2*n+2 - rectangle) % (2*n+2);
        if (corner == 1) return rectangle + 1;
        if (corner == 2) return rectangle + 2;

        // corner 3 
        return (2*n+1 - rectangle);
    }
};


class denali::rectangular::VerticalRectangleSplit : public OrientedRectangleSplit
{
public:
    virtual Rectangle::Point getBoundaryPoint(size_t i) const
    {
        unsigned int n = rectangles.size();

        if (i == n) return rectangles[n-1].se();
        if (i == 2*n+1) return rectangles[0].nw();
        if (i < n) return rectangles[i].sw();

        // i > n
        return rectangles[2*n-i].ne();
    }

    virtual size_t getIndexOfCorner(size_t i) const
    {
        unsigned int n = rectangles.size();

        if (i == 0) return 0;
        if (i == 1) return n;
        if (i == 2) return n+1;

        // i == 3
        return 2*n + 1;
    }

    size_t getIndexOfRectangleCorner(size_t rectangle, size_t corner) const
    {
        unsigned int n = rectangles.size();

        if (corner == 0) return rectangle;
        if (corner == 1) return rectangle + 1;
        if (corner == 2) return (2*n - rectangle);

        // corner == 3
        return (2*n - rectangle + 1);
    }
};


class denali::rectangular::RectangleSplit
{
    boost::shared_ptr<OrientedRectangleSplit> split;
public:
    RectangleSplit(boost::shared_ptr<OrientedRectangleSplit> split)
        : split(split) {}

    Rectangle getRectangle(size_t i) const
    {
        return split->getRectangle(i);
    }
    size_t numberOfRectangles() const
    {
        return split->numberOfRectangles();
    }

    Rectangle::Point operator[](size_t i) const
    {
        return (*split)[i];
    }

    size_t size() const
    {
        return split->size();
    }

    virtual Rectangle::Point getBoundaryPoint(size_t id) const
    {
        return split->getBoundaryPoint(id);
    }

    virtual size_t getIndexOfCorner(size_t i) const
    {
        return split->getIndexOfCorner(i);
    }

    virtual size_t getIndexOfRectangleCorner(size_t rectangle, size_t corner) const
    {
        return split->getIndexOfRectangleCorner(rectangle, corner);
    }
};


class denali::rectangular::RectangleSplitter
{
private:
    Rectangle rectangle;
    std::vector<double> weights;
    double sum_of_weights;
    bool horizontal_;

public:
    RectangleSplitter(Rectangle rectangle) :
        rectangle(rectangle), sum_of_weights(0), horizontal_(true)
    {}

    RectangleSplitter& horizontally() {
        horizontal_ = true;
        return *this;
    }
    RectangleSplitter& vertically() {
        horizontal_ = false;
        return *this;
    }

    RectangleSplitter& addWeight(const double weight)
    {
        if (weight <= 0)
            throw std::invalid_argument("Weight must be positive.");

        weights.push_back(weight);
        sum_of_weights += weight;

        return *this;
    }

    RectangleSplit split() const {
        if (horizontal_) {
            return splitHorizontally();
        } else {
            return splitVertically();
        }
    }

private:
    RectangleSplit splitHorizontally() const
    {
        boost::shared_ptr<HorizontalRectangleSplit> hsplit =
            boost::shared_ptr<HorizontalRectangleSplit>(new HorizontalRectangleSplit());

        double cursor_x = rectangle.sw().x();
        double cursor_y = rectangle.sw().y();
        double width = rectangle.width();
        for (size_t i=0; i<weights.size(); ++i) {
            double share = weights[i] / sum_of_weights;
            double height = share * rectangle.height();
            Rectangle new_rectangle(
                cursor_x + width/2,
                cursor_y + height/2,
                width,
                height);

            hsplit->addRectangle(new_rectangle);
            cursor_y = new_rectangle.nw().y();
        }

        return RectangleSplit(hsplit);
    }

    RectangleSplit splitVertically() const
    {
        boost::shared_ptr<VerticalRectangleSplit> vsplit =
            boost::shared_ptr<VerticalRectangleSplit>(new VerticalRectangleSplit());

        double cursor_x = rectangle.sw().x();
        double cursor_y = rectangle.sw().y();
        double height = rectangle.height();
        for (size_t i=0; i<weights.size(); ++i) {
            double share = weights[i] / sum_of_weights;
            double width = share * rectangle.width();
            Rectangle new_rectangle(
                cursor_x + width/2,
                cursor_y + height/2,
                width,
                height);

            vsplit->addRectangle(new_rectangle);
            cursor_x = new_rectangle.se().x();
        }

        return RectangleSplit(vsplit);
    }

};

////////////////////////////////////////////////////////////////////////////////
//
// Embedding
//
////////////////////////////////////////////////////////////////////////////////

/// \brief An embedding of the vertices of the landscape tree as points in R^3.
template <typename LandscapeTree>
class denali::rectangular::Embedding
{
public:
    class Point
    {
        double _x, _y, _z;
        unsigned int _id;

    public:
        Point(double x, double y, double z, unsigned int id)
            : _x(x), _y(y), _z(z), _id(id) { }

        double x() const {
            return _x;
        }
        double y() const {
            return _y;
        }
        double z() const {
            return _z;
        }
        unsigned int id() const {
            return _id;
        }
    };

private:
    typedef typename LandscapeTree::Node Node;
    typedef typename LandscapeTree::Arc Arc;
    typedef std::vector<Point> Points;

    const LandscapeTree& _tree;
    Points _points;
    StaticNodeMap<LandscapeTree, Points> _contour_points;
    StaticNodeMap<LandscapeTree, Points> _contour_corners;
    StaticNodeMap<LandscapeTree, Points> _contour_containers;

    Point _max_point;
    Point _min_point;

public:

    Embedding(const LandscapeTree& tree)
        : _tree(tree), _contour_points(tree), _contour_corners(tree),
          _contour_containers(tree), _max_point(0,0,0,0), _min_point(0,0,0,0) { }

    void insertCornerPoint(Point point, Node owner)
    {
        _contour_corners[owner].push_back(point);
    }

    void insertContainerPoint(Point point, Node owner)
    {
        _contour_containers[owner].push_back(point);
    }

    Point insertPoint(double x, double y, Node owner)
    {
        // make a new point
        unsigned int index = _points.size();
        Point point(x, y, _tree.getValue(owner), index);

        // insert the point into the vector of points
        _points.push_back(point);

        // add the point to the owner's list of points
        _contour_points[owner].push_back(point);

        // record the max and min point
        if (index == 0) {
            _min_point = point;
            _max_point = point;
        } else {
            if (point.z() < _min_point.z()) {
                _min_point = point;
            } else if (point.z() > _max_point.z()) {
                _max_point = point;
            }
        }

        return point;
    }

    void insertSplit(const RectangleSplit& split, Node owner)
    {
        // create a point for every point in the boundary
        std::vector<Point> inserted_points;

        for (size_t i=0; i<split.size(); ++i) {
            // get the boundary point
            Rectangle::Point boundary_point = split.getBoundaryPoint(i);

            // insert the new point
            Point point = insertPoint(boundary_point.x(), boundary_point.y(), owner);

            // keep track of the point
            inserted_points.push_back(point);
        }

        // keep track of the corners of the contour
        for (int i=0; i<4; ++i) {
            insertCornerPoint(inserted_points[split.getIndexOfCorner(i)], owner);
        }

        // associate each child of this node with its proper container
        unsigned int i = 0;
        for (ChildIterator<LandscapeTree> it(_tree, owner);
                !it.done(); ++it) {
            for (int j=0; j<4; ++j) {
                int index = split.getIndexOfRectangleCorner(i, j);
                insertContainerPoint(inserted_points[index], it.child());
            }
            ++i;
        }
    }

    Point getContourPoint(Node node, size_t i) const
    {
        return _contour_points[node][i];
    }

    Point getCornerPoint(Node node, size_t i) const
    {
        return _contour_corners[node][i];
    }

    Point getContainerPoint(Node node, size_t i) const
    {
        return _contour_containers[node][i];
    }

    size_t numberOfContourPoints(Node node) const
    {
        return _contour_points[node].size();
    }

    size_t numberOfCornerPoints(Node node) const
    {
        return _contour_corners[node].size();
    }

    size_t numberOfContainerPoints(Node node) const
    {
        return _contour_containers[node].size();
    }

    size_t numberOfPoints() const
    {
        return _points.size();
    }

    Point getPoint(size_t i) const
    {
        return _points[i];
    }

    Point getMaxPoint() const
    {
        return _max_point;
    }

    Point getMinPoint() const
    {
        return _min_point;
    }

};


/// \brief The arguments kept in a stack to unroll recursion.
template <typename LandscapeTree>
struct denali::rectangular::EmbeddingParameters
{
    typedef typename LandscapeTree::Arc Arc;
    Arc arc;
    Rectangle parent_rectangle;
    bool parent_orientation;

    EmbeddingParameters(Arc arc, Rectangle parent_rectangle, bool parent_orientation)
        : arc(arc), parent_rectangle(parent_rectangle), parent_orientation(parent_orientation)
    {}
};


template <typename LandscapeTree>
class denali::rectangular::Embedder
{
    const LandscapeTree& _tree;
    Embedding<LandscapeTree>& _embedding;
    const LandscapeWeights<LandscapeTree>& _weights;

    typedef typename LandscapeTree::Node Node;
    typedef typename LandscapeTree::Arc Arc;
    typedef EmbeddingParameters<LandscapeTree> Parameters;

public:
    Embedder(
        const LandscapeTree& tree,
        const LandscapeWeights<LandscapeTree>& weights,
        Embedding<LandscapeTree>& embedding)
        : _tree(tree), _embedding(embedding), _weights(weights) { }

    void embed()
    {
        // first, we make a rectangle for the root
        Rectangle root_rectangle(0,0,1,1);

        RectangleSplitter splitter(root_rectangle);

        // now split the current rectangle according to the total volumes of the children
        for (ChildIterator<LandscapeTree> child_it(_tree, _tree.getRoot());
                !child_it.done(); ++child_it) 
        {
            double edge_weight = _weights.getArcWeight(child_it.arc()) +
                                 _weights.getTotalNodeWeight(child_it.child());

            splitter.addWeight(edge_weight);
        }

        RectangleSplit split = splitter.split();
        _embedding.insertSplit(split, _tree.getRoot());

        // recursively embed the subtree
        std::stack<Parameters> embedding_stack;

        size_t i = 0;;
        for (ChildIterator<LandscapeTree> it(_tree, _tree.getRoot());
                !it.done(); ++it)
        {
            embedding_stack.push(Parameters(it.arc(), split.getRectangle(i), true));
            ++i;
        }

        while (!embedding_stack.empty())
        {
            Parameters params = embedding_stack.top();
            embedding_stack.pop();

            Node node = _tree.target(params.arc);

            if (_tree.outDegree(node) == 0) {
                // the child is a leaf
                embedLeaf(node, params.parent_rectangle);
            } else {
                // the child is a branch
                embedBranch(embedding_stack, 
                            params.arc, 
                            params.parent_rectangle, 
                            params.parent_orientation);
            }
        }
    }

private:
    void embedBranch(
        std::stack<Parameters>& embedding_stack,
        Arc arc,
        Rectangle parent_rectangle,
        bool split_vertically)
    {
        Node current = _tree.target(arc);

        // determine how much to shrink the rectangle
        double total_volume = _weights.getTotalNodeWeight(current);
        double arc_volume   = _weights.getArcWeight(arc);
        double shrink_ratio = total_volume / (total_volume + arc_volume + 1);

        Rectangle current_rectangle = parent_rectangle.shrink(shrink_ratio);

        // we'll split the rectangle in alternating directions on every
        // call to this function
        RectangleSplitter splitter(current_rectangle);
        if (split_vertically) {
            splitter.vertically();
        } else {
            splitter.horizontally();
        }

        // now split the current rectangle according to the total volumes of the children
        for (ChildIterator<LandscapeTree> child_it(_tree, current);
                !child_it.done(); ++child_it) {
            double edge_weight = _weights.getArcWeight(child_it.arc()) +
                                 _weights.getTotalNodeWeight(child_it.child());

            splitter.addWeight(edge_weight);
        }

        RectangleSplit split = splitter.split();
        _embedding.insertSplit(split, current);

        // recursively embed the subtree
        size_t i = 0;;
        for (ChildIterator<LandscapeTree> it(_tree, current);
                !it.done(); ++it)
        {
            embedding_stack.push(Parameters(it.arc(), split.getRectangle(i), !split_vertically));
            ++i;
        }
    }

    void embedLeaf(Node current, Rectangle parent_rectangle)
    {
        Rectangle::Point center = parent_rectangle.center();
        _embedding.insertPoint(center.x(), center.y(), current);
    }
};

////////////////////////////////////////////////////////////////////////////////
//
// Triangularization
//
////////////////////////////////////////////////////////////////////////////////

template <typename LandscapeTree>
class denali::rectangular::Triangularization
{
public:
    class Triangle
    {
        friend class Triangularization;
        unsigned int _i, _j, _k;
        unsigned int _id;

    public:
        Triangle(unsigned int i, unsigned int j, unsigned int k, size_t id)
            : _i(i), _j(j), _k(k), _id(id) { }

        unsigned int i() const {
            return _i;
        }

        unsigned int j() const {
            return _j;
        }

        unsigned int k() const {
            return _k;
        }
        
        unsigned int id() const {
            return _id;
        }
    };

private:
    typedef typename LandscapeTree::Node Node;
    typedef typename LandscapeTree::Arc Arc;

    std::vector<Triangle> _triangles;
    std::vector<Arc> _arcs;

public:

    void insertTriangle(unsigned int i, unsigned int j, unsigned int k, Arc arc)
    {
        _triangles.push_back(Triangle(i,j,k,_triangles.size()));
        _arcs.push_back(arc);
    }

    Arc getArc(Triangle tri) const
    {
        return _arcs[tri._id];
    }

    size_t numberOfTriangles() const
    {
        return _triangles.size();
    }

    Triangle getTriangle(size_t i) const
    {
        return _triangles[i];
    }

};


template <typename LandscapeTree>
class denali::rectangular::Triangularizer
{
    typedef typename LandscapeTree::Node Node;
    typedef typename LandscapeTree::Arc Arc;
    typedef rectangular::Triangularization<LandscapeTree>
    Triangularization;

    const LandscapeTree& _tree;
    const Embedding<LandscapeTree>& _embedding;
    Triangularization& _triangularization;

public:
    Triangularizer(
        const LandscapeTree& tree,
        const Embedding<LandscapeTree>& embedding,
        Triangularization& triangularization)
        : _tree(tree), _embedding(embedding),
          _triangularization(triangularization) {}

    void triangularize()
    {

        // simulate recursion with a stack
        std::stack<Arc> triangularization_stack;

        // now recurse
        for (ChildIterator<LandscapeTree> it(_tree, _tree.getRoot());
                !it.done(); ++it)
        {
            triangularization_stack.push(it.arc());
        }

        while (!triangularization_stack.empty())
        {
            Arc arc = triangularization_stack.top();
            triangularization_stack.pop();

            Node node = _tree.target(arc);

            if (_tree.outDegree(node) == 0)
            {
                triangularizeLeafArc(arc);
            } 
            else 
            {
                triangularizeBranchArc(triangularization_stack, arc);
            }
        }
    }

private:

    void triangularizeBranchArc(
            std::stack<Arc>& triangularization_stack, 
            Arc arc)
    {
        triangulateNestedRectangle(arc);

        for (ChildIterator<LandscapeTree> it(_tree, _tree.target(arc));
                !it.done(); ++it) 
        {
            triangularization_stack.push(it.arc());
        }
    }

    void triangularizeLeafArc(Arc arc)
    {
        triangulateNestedPoint(arc);
    }

    void triangulateNestedRectangle(Arc arc)
    {
        Node inner = _tree.target(arc);

        for (int i=0; i<4; ++i) {
            _triangularization.insertTriangle(
                _embedding.getContainerPoint(inner, i).id(),
                _embedding.getContainerPoint(inner, (i+1)%4).id(),
                _embedding.getCornerPoint(inner, i).id(),
                arc);
        }

        for (int i=0; i<4; ++i) {
            _triangularization.insertTriangle(
                _embedding.getCornerPoint(inner, i).id(),
                _embedding.getCornerPoint(inner, (i+1)%4).id(),
                _embedding.getContainerPoint(inner, (i+1)%4).id(),
                arc);
        }
    }

    void triangulateNestedPoint(Arc arc)
    {
        Node inner = _tree.target(arc);

        for (int i=0; i<4; ++i) {
            _triangularization.insertTriangle(
                _embedding.getContainerPoint(inner, i).id(),
                _embedding.getContainerPoint(inner, (i+1)%4).id(),
                _embedding.getContourPoint(inner, 0).id(),
                arc);
        }
    }

};

////////////////////////////////////////////////////////////////////////////////
//
// RectangularLandscape
//
////////////////////////////////////////////////////////////////////////////////

/// \brief An embedding of the contour tree as a rectangular landscape.
/// \ingroup landscape
template <typename ContourTree>
class denali::RectangularLandscape :
    public
    ReadableDirectedGraphMixin < LandscapeTree <ContourTree> ,
    BaseGraphMixin < LandscapeTree <ContourTree> > >
{
    typedef
    ReadableDirectedGraphMixin < denali::LandscapeTree <ContourTree> ,
                               BaseGraphMixin < denali::LandscapeTree <ContourTree> > >
                               Mixin;

    typedef denali::LandscapeTree<ContourTree> LandscapeTree;
    typedef denali::LandscapeWeights<LandscapeTree> LandscapeWeights;
    typedef denali::rectangular::Embedding<LandscapeTree> Embedding;
    typedef denali::rectangular::Triangularization<LandscapeTree> Triangularization;

    LandscapeTree _tree;
    LandscapeWeights _weights;
    Embedding _embedding;
    Triangularization _triangularization;

    // prevent copying
    RectangularLandscape(const RectangularLandscape&);
    RectangularLandscape& operator=(const RectangularLandscape&);

    void buildLandscape()
    {
        // create an embedder
        rectangular::Embedder<LandscapeTree> embedder(_tree, _weights, _embedding);
        embedder.embed();

        // create a triangularizer
        rectangular::Triangularizer<LandscapeTree>
        triangularizer(_tree, _embedding, _triangularization);
        triangularizer.triangularize();
    }

public:
    
    typedef typename LandscapeTree::Node Node;
    typedef typename LandscapeTree::Arc Arc;

    typedef typename Embedding::Point Point;
    typedef typename Triangularization::Triangle Triangle;

    RectangularLandscape(
        const ContourTree& tree,
        typename ContourTree::Node root)
        : Mixin(_tree), _tree(tree, root), _weights(_tree), _embedding(_tree)
    {
        buildLandscape();
    }

    RectangularLandscape(
        const ContourTree& tree,
        typename ContourTree::Node root,
        WeightMap* weight_map)
        : Mixin(_tree), _tree(tree, root), _weights(_tree, weight_map), _embedding(_tree)
    {
        buildLandscape();
    }

    /// \brief Returns the number of points in the embedding.
    size_t numberOfPoints() const {
        return _embedding.numberOfPoints();
    }

    /// \brief Gets the point at an index.
    Point getPoint(size_t index) const {
        return _embedding.getPoint(index);
    }

    /// \brief Retrieves the point with the greatest z value.
    Point getMaxPoint() const {
        return _embedding.getMaxPoint();
    }

    /// \brief Retrieves the point with the least z value.
    Point getMinPoint() const {
        return _embedding.getMinPoint();
    }

    /// \brief Returns the number of triangles in the embedding.
    size_t numberOfTriangles() const {
        return _triangularization.numberOfTriangles();
    }

    /// \brief Gets the triangle at the index.
    Triangle getTriangle(size_t index) const {
        return _triangularization.getTriangle(index);
    }

    /// \brief Gets the arc (component) that a triangle represents.
    Arc getComponentFromTriangle(Triangle tri) const {
        return _triangularization.getArc(tri);
    }

    /// \brief Retrieves the arc (component) from the arc's identifier.
    Arc getComponentFromIdentifier(unsigned int identifier) const {
        return this->getArcFromIdentifier(identifier);
    }

    /// \brief Returns the weight of the component.
    double getComponentWeight(Arc arc) const {
        return _weights.getArcWeight(arc);
    }

    /// \brief Returns the total weight of the component.
    double getTotalNodeWeight(Node node) const {
        return _weights.getTotalNodeWeight(node);
    }

    /// \brief Returns the contour tree node that corresponds to this landscape node.
    typename ContourTree::Node getContourTreeNode(Node node) const {
        return _tree.getContourTreeNode(node);
    }

    /// \brief Returns the edge in the contour tree that corresponds to this arc.
    typename ContourTree::Edge getContourTreeEdge(Arc arc) const {
        return _tree.getContourTreeEdge(arc);
    }

    /// \brief Given a node in the contour tree, returns the corresponding node
    /// from the landscape.
    Node getLandscapeTreeNode(typename ContourTree::Node node) const {
        return _tree.getLandscapeTreeNode(node);
    }

    /// \brief Given an edge in the contour tree, returns the corresponding arc
    /// from the landscape.
    Node getLandscapeTreeArc(typename ContourTree::Edge edge) const {
        return _tree.getLandscapeTreeArc(edge);
    }

    /// \brief Returns the root of the landscape.
    Node getRoot() const {
        return _tree.getRoot();
    }

};


/// \brief A builder of rectangular landscapes.
template <typename ContourTree>
class denali::RectangularLandscapeBuilder
{

public:
    typedef RectangularLandscape<ContourTree> LandscapeType;

    static LandscapeType* build(
            const ContourTree& contour_tree,
            typename ContourTree::Node root)
    {
        if (!contour_tree.isNodeValid(root)) {
            throw std::runtime_error("Invalid root given for landscape generation.");
        }

        return new LandscapeType(contour_tree, root);
    }

    static LandscapeType* build(
            const ContourTree& contour_tree,
            typename ContourTree::Node root,
            WeightMap* weight_map)
    {
        if (!contour_tree.isNodeValid(root)) {
            throw std::runtime_error("Invalid root given for landscape generation.");
        }

        return new LandscapeType(contour_tree, root, weight_map);
    }


};

#endif
