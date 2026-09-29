#include "CreatePageAction.h"

CreatePageAction::CreatePageAction(NewPagePort &newPagePort)
    : newPagePort(newPagePort) {}

TuplePageId CreatePageAction::execute() { return newPagePort.newPage(); }
