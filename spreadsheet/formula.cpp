#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    if (fe.GetCategory() == FormulaError::Category::Div0) {
        return output << "#ARITHM!";
    }
    else if (fe.GetCategory() == FormulaError::Category::Value) {
        return output << "#VALUE!";
    }
    //return output << "#ARITHM!";
    return output << "#REF!";
}



namespace {
    struct PositionHasher {
        size_t operator()(const Position& pos) const {
            return static_cast<size_t>(pos.row + 100000 * pos.col);
        }
    };

    class Formula : public FormulaInterface {
    public:
    // Реализуйте следующие методы:
        explicit Formula(std::string expression) try : ast_(ParseFormulaAST(expression)) {
        
        }
        catch (...) {
            throw  FormulaException("Wrong syntax: could not parse formula.");
        }

        std::string GetExpression() const override {
            std::ostringstream stream;
            ast_.PrintFormula(stream);
            return stream.str();
        }

        //***IMPLEMENT THIS METHOD***
        Value Evaluate(const SheetInterface& sheet) const override {
            //Add all exception handling here
            double result = 0;
            try {
                result = ast_.Execute(sheet);
                return result;
            }
            catch (const FormulaError& error) {
                return error;
            }
            //return result;
        }

        //Give back references cells:
        //*SORTED
        //*UNIQUE
        std::vector<Position> GetReferencedCells() const override {
            //std::unordered_set<Position,PositionHasher> unique_cells;
            std::set<Position> unique_cells;
            for (auto cell : ast_.GetCells()) {
                unique_cells.insert(cell);
            }
            return std::vector<Position>(unique_cells.begin(), unique_cells.end());
        }


    private:
        FormulaAST ast_;
    };
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}