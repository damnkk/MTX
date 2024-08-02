#ifndef SCENEGRAPH_H
#define SCENEGRAPH_H
#include "glm/common.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <algorithm>
#include <string>
#include <vector>

namespace MTX {
struct Hierarchy {
  Hierarchy() {}
  Hierarchy(int pParent, int pFirstChild, int pNextSibling, int pLastSibling, int pLevel)
      : parent(pParent), firstChild(pFirstChild), nextSibling(pNextSibling),
        lastSibling(pLastSibling), level(pLevel) {}
  int parent = -1;
  int firstChild = -1;
  int nextSibling = -1;
  int lastSibling = -1;
  int level = 0;
};

struct SceneGraph {
  int  addNode(int parent, int level);
  int  addNode(int parent, int level, std::string nodeName);
  void collectNodesToDelete(int node, std::vector<uint32_t>& nodes);
  // resource loader may execute scene again, after delecting the node of scene
  void        deleteSceneNodes(const std::vector<uint32_t>& nodesToDelete);
  void        markAsChanged(int node);
  void        recalculateAllTransforms();
  std::string getNodeName(int node) const;

 public:
  const glm::mat4& getGlobalTransformsFromIdx(uint32_t idx) { return m_globalTransforms[idx]; }

  std::vector<std::vector<int>>          m_changedAtThisFrame = std::vector<std::vector<int>>(32);
  std::vector<Hierarchy>                 m_nodeHierarchy;
  std::vector<glm::mat4>                 m_localTransforms;
  std::vector<glm::mat4>                 m_globalTransforms;
  std::vector<glm::vec3>                 m_rotateRecord;
  std::unordered_map<uint32_t, uint32_t> m_meshMap;
  std::unordered_map<uint32_t, uint32_t> m_materialMap;
  std::unordered_map<uint32_t, uint32_t> m_boundingBoxMap;
  std::unordered_map<uint32_t, uint32_t> m_nameForNodeMap;
  std::vector<std::string>               m_nodeNames;

  int maxLevel = -1;
};

//if any item in v is founded in selection array, the item will be remove to end of the v;
template<class T, class Index = uint32_t>
void eraseSelected(std::vector<T>& v, const std::vector<Index>& selection) {
  v.resize(std::distance(
      v.begin(), std::stable_partition(v.begin(), v.end(), [&selection, &v](const T& item) {
        return !std::binary_search(selection.begin(), selection.end(),
                                   static_cast<Index>(static_cast<const T*>(&item) - &v[0]));
      })));
}
}// namespace MTX

#endif//SCENEGRAPH_H