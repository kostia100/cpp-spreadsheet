#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

// Реализуйте следующие методы

//Graph
Graph::Graph() {};

Graph::Graph(const Graph& other)
	: vertex_to_childs_(other.vertex_to_childs_)
	, vertices_(other.vertices_) {
}

void Graph::Swap(Graph& other) {
	std::swap(vertex_to_childs_, other.vertex_to_childs_);
	std::swap(vertices_, other.vertices_);
}

void Graph::AddEdge(Position start, Position end) {
	if (std::count(vertices_.begin(), vertices_.end(), start) == 0) {
		vertices_.push_back(start);
	}

	if (std::count(vertices_.begin(), vertices_.end(), end) == 0) {
		vertices_.push_back(end);
	}

	if (vertex_to_childs_.find(start) != vertex_to_childs_.end()) {
		std::vector<Position>& childs = vertex_to_childs_[start];
		if (count(childs.begin(), childs.end(), end) == 0) {
			childs.push_back(end);
		}
	}
	else {
		vertex_to_childs_[start] = { end };
	}
}

void Graph::RemoveEdge(Position start, Position end) {
	std::vector<Position>& childs = vertex_to_childs_[start];
	auto it = std::find(childs.begin(), childs.end(), end);
	if (it != childs.end()) {
		childs.erase(it);
	}
}

bool Graph::IsCyclicRecursive(
	Position current_vertex,
	std::unordered_map<Position, bool, PositionHasher>& visited,
	std::unordered_map<Position, bool, PositionHasher>& recStack) const {
	if (visited[current_vertex] == false) {
		visited[current_vertex] = true;
		recStack[current_vertex] = true;

		if (vertex_to_childs_.find(current_vertex) != vertex_to_childs_.end()) {

			std::vector<Position> childs = vertex_to_childs_.at(current_vertex);
			for (auto vertex : childs) {
				if (!visited[vertex] && IsCyclicRecursive(vertex, visited, recStack)) {
					return true;
				}
				else if (recStack[vertex] == true) {
					return true;
				}
			}
		}

	}
	recStack[current_vertex] = false;
	return false;
}

bool Graph::IsCyclic() const {
	std::unordered_map<Position, bool, PositionHasher> visited;
	std::unordered_map<Position, bool, PositionHasher> recStack;
	for (auto vertex : vertices_) {
		visited[vertex] = false;
		recStack[vertex] = false;
	}

	for (auto vertex : vertices_) {
		if (!visited[vertex] && IsCyclicRecursive(vertex, visited, recStack)) {
			return true;
		}
	}
	return false;
}


void Graph::TranverseGraphAndInvalidateCache(
	Position vertex,
	std::unordered_map< Position, std::optional<CellInterface::Value>, PositionHasher>& cache_storage) {
	std::unordered_map<Position, bool, PositionHasher> visited;
	auto nullify_vertex = [&cache_storage](Position vertex) {
		cache_storage[vertex] = std::nullopt;
		};
	DFS(vertex, visited, nullify_vertex);
}

//Dependencies Manager

bool DependenciesManager::TryAddNewVertex(Position vertex, std::vector<Position> parents) {
	if (parents.size() == 0) {
		//no-dependencies
		return true;
	}

	Graph tmp_grap(dependencies_graph);
	for (Position parent : parents) {
		tmp_grap.AddEdge(parent, vertex);
	}
	if (tmp_grap.IsCyclic()) {
		return false;
	}
	else {
		dependencies_graph.Swap(tmp_grap);
		vertex_to_parents_[vertex] = parents;
		return true;
	}
}

bool DependenciesManager::TryUpdateVertex(Position vertex, std::vector<Position> parents) {
	Graph tmp_grap(dependencies_graph);
	//get current parents
	std::vector<Position> current_parents;
	if (vertex_to_parents_.find(vertex) != vertex_to_parents_.end()) {
		current_parents = vertex_to_parents_[vertex];
	}
	//remove current parents from copy
	for (Position current_parent : current_parents) {
		tmp_grap.RemoveEdge(current_parent, vertex);
	}
	//Add new parents
	for (Position parent : parents) {
		tmp_grap.AddEdge(parent, vertex);
	}
	//check if copy is isCyclic
	if (tmp_grap.IsCyclic()) {
		//copy_graph IS cyclic => stop operation/throw exception 
		return false;
	}
	else {
		//copy_graph IS OK 
		// 1.Update/swap graph.
		// 2.Invalidate cache.
		// 3.Update new parents.
		dependencies_graph.Swap(tmp_grap);
		InvalidateCache(vertex);
		vertex_to_parents_[vertex] = parents;
		return true;
	}
}

bool DependenciesManager::IsInCache(Position pos) const {
	if (vertex_to_cache_.find(pos) == vertex_to_cache_.end()) {
		return false;
	}
	if (vertex_to_cache_.at(pos) == std::nullopt) {
		return false;
	}
	return true;
}

CellInterface::Value DependenciesManager::GetCache(Position pos) const {
	return *vertex_to_cache_.at(pos);
}

void DependenciesManager::AddToCache(Position pos, CellInterface::Value value, std::vector<Position> parents) {
	vertex_to_cache_[pos] = value;
}

void DependenciesManager::InvalidateCache(Position vertex) {
	dependencies_graph.TranverseGraphAndInvalidateCache(vertex, vertex_to_cache_);
}


//Types of cells 

Impl::Impl(std::string expression) :expression_(expression) {
}

std::string Impl::GetText() const {
	return expression_;
}

std::vector<Position> Impl::GetReferencedCells() const {
	return {};
}



EmptyImpl::EmptyImpl() : Impl("") {

}

Impl::ImplValue EmptyImpl::GetValue() const {
	return "";
}

TextImpl::TextImpl(std::string text) : Impl(text) {

}

Impl::ImplValue TextImpl::GetValue() const {
	if (expression_[0] == '\'') {
		return expression_.substr(1);
	}
	return expression_;
}

FormulaImpl::FormulaImpl(std::string formula, const SheetInterface& sheet)
	: Impl(formula)
	, sheet_(sheet)			{
	formula_ = ParseFormula(expression_.substr(1));
}

Impl::ImplValue FormulaImpl::GetValue() const {
	std::variant<double, FormulaError> evaluation = formula_->Evaluate(sheet_);
	if (auto* value = std::get_if<double>(&evaluation)) {
		return *value;
	}
	else {
		auto* except = std::get_if<FormulaError>(&evaluation);
		return *except;
	}
}

std::string FormulaImpl::GetText() const {
	return "=" + formula_->GetExpression();
}

std::vector<Position> FormulaImpl::GetReferencedCells() const {
	return formula_->GetReferencedCells();
}



Cell::~Cell() {
}

Cell::Cell(const SheetInterface& sheet, DependenciesManager& manager, Position pos)
	: sheet_(sheet)
	, dependencies_manager_(manager)
	, pos_(pos) {
}

void Cell::Set(std::string text) {
	if (text.size() == 0) {
		impl_ = std::make_unique<EmptyImpl>();
	}
	else if (text[0] != '=' || (text[0] == '=' && text.size() == 1) ) {
		impl_ = std::make_unique<TextImpl>(text);
	}
	else {
		impl_ = std::make_unique<FormulaImpl>(text,sheet_);
	}
}

void Cell::Clear() {
	impl_ = std::make_unique<EmptyImpl>();
}


CellInterface::Value Cell::GetValue() const {
	if (dependencies_manager_.IsInCache(pos_)) {
		return dependencies_manager_.GetCache(pos_);
	}
	CellInterface::Value value = impl_->GetValue();
	dependencies_manager_.AddToCache(pos_,value,impl_->GetReferencedCells());
	return value;
}

std::string Cell::GetText() const {
	return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
	return impl_->GetReferencedCells();
}