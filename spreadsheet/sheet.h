#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <vector>

class Sheet : public SheetInterface {
public:
    ~Sheet();

    Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

	// Можете дополнить ваш класс нужными полями и методами


private:
	// Можете дополнить ваш класс нужными полями и методами
    

    //Check if the position is valid and throw an exception otherwise.
    static void CheckIfPositionIsValid(Position pos);

    //Update the printable after deleting cell.
    void UpdatePrintableZoneAfterClearingCell(Position pos);

    //Check if the formula does not lead to circular dependencies.
    //Invalidate cache if we update an already existing cell.
    //bool ValidDependencies(Position pos, std::string text);

    bool IsInGrid(Position pos) const;
    void AddRowsToGrid(int n);
    void AddColumsToGrid(int n);

    //Create a cell in the grid with the text.
    void SetCellInGrid(Position pos, std::string text);
    //Create dependent empty cells.
    void SetDependentCells(Position pos);
    
    //Traverse the printable zone and apply operation.
    template <typename Func>
    void VisitPrintableZone(std::ostream& output, Func operation) const;

    // *first dimension: rows
    // *second dimension: columns
    std::vector<std::vector<std::shared_ptr<Cell>>> cells_ ;
    
    Size printable_size_;

    DependenciesManager dependencies_manager;

};


template <typename Func>
void Sheet::VisitPrintableZone(std::ostream& output, Func operation) const {
    using namespace std::literals;
    for (int r = 0; r < printable_size_.rows; ++r) {
        bool is_first = true;
        for (int c = 0; c < printable_size_.cols; ++c) {
            if (is_first) {
                is_first = false;
            }
            else {
                //output << '\t';
                output << "\t"sv;
            }
            if (cells_[r][c] != nullptr) {
                operation(cells_[r][c]);
            }
        }
        //output << '\n';
        output << "\n"sv;
    }
}
