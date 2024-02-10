#pragma once

#include "common.h"
#include "formula.h"
#include "unordered_map"
#include "optional"
#include <algorithm>

//Hasher of a Position instance: needed in the graph+cache implementation.

struct PositionHasher {
    size_t operator()(const Position& pos) const {
        return static_cast<size_t>(pos.row + 100000 * pos.col);
    }
};

//Implementation of a Graph:
// * Has a DFS traversal.
// * Has a Cyclicity check.
// Has Copy-Ctor + Swap = when we add a new edge/vertex:
// 1. Make a copy of the graph.
// 2. Add the edge/vertex.
// 3. Check if the new graph is cyclic.
// 4. If not cyclic, swap new and current.
class Graph {
public:
    //Ctor.
    Graph();

    //Copy ctor:
    Graph(const Graph& other);

    //Swap the data of two graphs.
    void Swap(Graph& other);

    //Add edge: the end position contain the start in its formula.
    //Start: parent.
    //End: child.
    void AddEdge(Position start, Position end);

    //When the data is invalidated.
    void RemoveEdge(Position start, Position end);

    bool IsCyclicRecursive(
        Position current_vertex,
        std::unordered_map<Position, bool, PositionHasher>& visited,
        std::unordered_map<Position, bool, PositionHasher>& recStack) const;

    //Check if the graph is cyclic
    bool IsCyclic() const;

    //Traverse the graph in Depth-First-Search and apply the method func to 
    //each traversed node.
    template<typename Func>
    void DFS(
        Position vertex,
        std::unordered_map<Position, bool, PositionHasher>& visited,
        Func func);

    //Traverse the graph starting from the invalidated vertex
    //and invalidate the  
    void TranverseGraphAndInvalidateCache(
        Position vertex,
        std::unordered_map< Position, std::optional<CellInterface::Value>, PositionHasher>& cache_storage);

private:
    //main graph data
    std::unordered_map< Position, std::vector<Position>, PositionHasher> vertex_to_childs_;
    std::vector<Position> vertices_;
};


template<typename Func>
void Graph::DFS(
    Position vertex,
    std::unordered_map<Position, bool, PositionHasher>& visited,
    Func func) {

    visited[vertex] = true;
    func(vertex);

    if (vertex_to_childs_.find(vertex) != vertex_to_childs_.end()) {

        std::vector<Position> childs = vertex_to_childs_.at(vertex);
        for (auto vertex : childs) {
            if (!visited[vertex]) {
                DFS(vertex, visited, func);
            }
        }
    }
}

/// <summary>
/// Stores the dependencies between the cells:
/// 1. Keep track of the dependencies of the cells.
/// 2. Keep track of the values in the cache.
/// </summary>
class DependenciesManager {
public:
    //Return true: do not lead to cyclic dependencies => add new vertex to graph.
    //Return false: the addition will lead to a cycle, leave graph intact.
    bool TryAddNewVertex(Position vertex, std::vector<Position> parents);

    //Here, we are updating an already existing vertex in the graph.
    //Possible modification
    bool TryUpdateVertex(Position vertex, std::vector<Position> parents);

    //Check if pos has a value in the cache.
    bool IsInCache(Position pos) const;

    CellInterface::Value GetCache(Position pos) const;

    //Add the value in the cache for position.
    void AddToCache(Position pos, CellInterface::Value value, std::vector<Position> parents);

    //When a vertex is invalidated:
    //* Remove the edges betwen the vertex and its **parents** from the dependencies.
    //* Remove the value from the cache.
    void InvalidateCache(Position vertex);

private:

    //Graph to check for cyclic dependencies.
    Graph dependencies_graph;

    //Need to keep track of the parents for cache-invalidation.
    std::unordered_map< Position, std::vector<Position>, PositionHasher> vertex_to_parents_;

    //Value of the cache.
    std::unordered_map< Position, std::optional<CellInterface::Value>, PositionHasher> vertex_to_cache_;
};






//TYPES OF CELLS
class Impl {
public:
    using ImplValue = std::variant<std::string, double, FormulaError>;
    
    Impl(std::string expression);
    virtual ImplValue GetValue() const = 0;
    virtual std::string GetText() const;
    virtual std::vector<Position> GetReferencedCells() const;

protected:
    std::string expression_;
};

class EmptyImpl : public Impl {
public:
    EmptyImpl();
    ImplValue GetValue() const override;
};

class TextImpl : public Impl {
public:
    TextImpl(std::string text);
    ImplValue GetValue() const override;
};

class FormulaImpl : public Impl {
public:
    FormulaImpl(std::string formula, const SheetInterface& sheet);
    ImplValue GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
    /*
    std::vector<Position> GetReferencedCells() const override {
        return formula_->GetReferencedCells();
    }
    */

private:
    std::unique_ptr<FormulaInterface> formula_;
    const SheetInterface& sheet_;
};




class Cell : public CellInterface {
public:
    ~Cell();

    Cell(const SheetInterface& sheet, DependenciesManager& manager, Position pos);
        /*
        : sheet_(sheet)
        , dependencies_manager_(manager)
        , pos_(pos) {
    }
    */

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
    /*
    std::vector<Position> GetReferencedCells() const override {
        return impl_->GetReferencedCells();
    }
    */

private:
    std::unique_ptr<Impl> impl_;
    const SheetInterface& sheet_;
    DependenciesManager& dependencies_manager_;
    Position pos_;
};