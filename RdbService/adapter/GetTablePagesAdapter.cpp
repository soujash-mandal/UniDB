#include "GetTablePagesAdapter.h"
#include "../../SpaceMapService/actions/GetTablePagesAction.h"
GetTablePagesAdapter::GetTablePagesAdapter(GetTablePagesAction &action)
    : action(action) {}
std::vector<uint32_t>
GetTablePagesAdapter::getTablePages(uint32_t rootSpaceMapPageId) {
  return action.execute(rootSpaceMapPageId);
}
