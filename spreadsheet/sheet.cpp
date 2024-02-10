#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {

}


Sheet::Sheet(){
    printable_size_ = { 0,0 };
}

void Sheet::AddRowsToGrid(int missing_rows) {
    for (int a = 1; a <= missing_rows; ++a) {
        int grid_columns = cells_.size() == 0 ? 0 : cells_[0].size();
        std::vector<std::shared_ptr<Cell>> empty_row(grid_columns, nullptr);
        cells_.push_back(empty_row);
    }
}

void Sheet::AddColumsToGrid(int missing_columns) {
    for (size_t a = 0; a < cells_.size(); ++a) {
        for (int b = 1; b <= missing_columns; ++b) {
            cells_[a].push_back(nullptr);
        }
    }
}

void Sheet::SetCellInGrid(Position pos, std::string text) {
    if (cells_[pos.row][pos.col] == nullptr) {
        cells_[pos.row][pos.col] = std::make_shared<Cell>(*this, this->dependencies_manager,pos);
    }
    cells_[pos.row][pos.col]->Set(text);
}


bool Sheet::ValidPosition(Position pos) {
    if (pos.row < 0 || pos.col < 0) {
        return false;
    }
    if (pos.row >= Position::MAX_ROWS ||pos.col >= Position::MAX_COLS ) {
        return false;
    }
    return true;
}

bool Sheet::ValidDependencies(Position pos, std::string text) {
    std::shared_ptr<Cell> cell_ptr = std::make_shared<Cell>(*this,this->dependencies_manager, pos);
    cell_ptr->Set(text);
    std::vector<Position> parents = cell_ptr->GetReferencedCells();
    CellInterface* current_cell = GetCell(pos);
    if (current_cell == nullptr) {
        //this is a new cell, no invalidation possible
        return dependencies_manager.TryAddNewVertex(pos, parents);
    }
    //here we are overwriting an already existing cell
    //need to invalidate cash
    return dependencies_manager.TryUpdateVertex(pos, parents);
    //return dependencies_manager.TryAddNewVertex(pos, parents);
}

bool Sheet::IsInGrid(Position pos) const {
    if (pos.row < 0 || pos.col < 0) {
        return false;
    }
    int grid_rows = cells_.size();
    int grid_cols = cells_.size() == 0 ? 0 : cells_[0].size();
    if (pos.row >= grid_rows || pos.col >= grid_cols) {
        return false;
    }
    return true;
}

void Sheet::SetCell(Position pos, std::string text) {
    if (!ValidPosition(pos)) {
        throw InvalidPositionException("Invalid position of cell");
    }
    if (!ValidDependencies(pos,text)) {
        throw CircularDependencyException("Circular dependency");
    }


    bool is_in_printable_zone = (pos.row < printable_size_.rows ) && (pos.col < printable_size_.cols);
    if(is_in_printable_zone) {
        SetCellInGrid(pos, text);
    }
    else {
        int grid_rows = cells_.size();
        int grid_columns = cells_.size() == 0 ? 0 : cells_[0].size();
        bool is_in_grid = (pos.row < grid_rows) && (pos.col < (grid_columns));
        if (is_in_grid) {
            SetCellInGrid(pos, text);
            printable_size_.rows = std::max(printable_size_.rows, pos.row + 1);
            printable_size_.cols = std::max(printable_size_.rows, pos.col + 1);
        }
        else {
            int missing_rows = pos.row - grid_rows + 1;
            int missing_columns = pos.col - grid_columns + 1;

            if (missing_rows > 0) {
                AddRowsToGrid(missing_rows);
                printable_size_.rows = pos.row + 1;
            }
            if (missing_columns > 0) {
                AddColumsToGrid(missing_columns);
                printable_size_.cols = pos.col + 1;
            }
            SetCellInGrid(pos, text);
        }
    }
    SetDependentCells(pos);
}

//A cell can have dependent cells. They also need to be added to
//the sheet (as empty) if they do not exist.
void Sheet::SetDependentCells(Position pos) {
    //the cell at pos has just been created.
    std::vector<Position> cells_references = GetCell(pos)->GetReferencedCells();
    for (Position pos_cell : cells_references) {
        if (GetCell(pos_cell)==nullptr) {
            SetCell(pos_cell,"");
        }
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!ValidPosition(pos)) {
        throw InvalidPositionException("Invalid position of cell");
    }
    if (!IsInGrid(pos)) {
        return nullptr;
    }
    if (cells_[pos.row][pos.col] == nullptr) {
        return nullptr;
    }

    return cells_[pos.row][pos.col].get();
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!ValidPosition(pos)) {
        throw InvalidPositionException("Invalid position of cell");
    }

    if (!IsInGrid(pos)) {
        return nullptr;
    }
    if (cells_[pos.row][pos.col] == nullptr) {
        return nullptr;
    }
    return cells_[pos.row][pos.col].get();
}

void Sheet::ClearCell(Position pos) {
    if (!ValidPosition(pos)) {
        throw InvalidPositionException("Invalid position of cell");
    }
    if (!IsInGrid(pos)) {
        return;
    }

    if (pos.row < printable_size_.rows && pos.col < printable_size_.cols) {
        cells_[pos.row][pos.col] = nullptr;
        //update size...
        if (pos.row == printable_size_.rows -1 ) {
            int current_row = printable_size_.rows - 1;
            while (current_row >=0) {
                
                int full_cells = count_if(cells_[current_row].begin(), cells_[current_row].end(), [](auto cell_ptr) {
                    return cell_ptr != nullptr;
                    });

                if (full_cells >= 1) {
                    printable_size_.rows = current_row + 1 ;
                    break;
                }
                --current_row;
            }
            if (current_row < 0) {
                //all rows are empty
                printable_size_.rows = 0;
                printable_size_.cols = 0;
                return;
            }
        }
        if (pos.col == printable_size_.cols -1) {
            int current_col = printable_size_.cols - 1;
            while (current_col >= 0) {
                int full_cells = 0;
                for (size_t r = 0; r < cells_.size() ; ++r) {
                    if (cells_[r][current_col]!=nullptr) {
                        ++full_cells;
                        break;
                    }
                }
                if (full_cells >= 1) {
                    printable_size_.cols = current_col + 1;
                    break;
                }
                --current_col;
            }
        }
    }
}



Size Sheet::GetPrintableSize() const {
    return printable_size_;
}

void Sheet::PrintValues(std::ostream& output) const {
    VisitPrintableZone(output, [&output](auto cell_ptr) {
        auto value = cell_ptr->GetValue();
        std::visit(
            [&](const auto& x) {
                output << x;
            },
            value);
        });
}

void Sheet::PrintTexts(std::ostream& output) const {
    VisitPrintableZone(output, [&output](auto cell_ptr) {
        std::string val = cell_ptr->GetText();
        output << val;
        });
}


std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}