#ifndef KOUEK_POINT_OCTREE_H
#define KOUEK_POINT_OCTREE_H

#include <array>
#include <iostream>
#include <queue>
#include <stack>

#include <glm/gtc/matrix_transform.hpp>

namespace kouek {
template <typename VertDatTy> class PointOctree {
  public:
    template <typename VertDatTy> struct Node {
        static constexpr uint8_t DEFAULT_DAT_CAP = 2;

        glm::vec3 min, max;
        uint8_t datNum = 0, datCap = DEFAULT_DAT_CAP;
        std::pair<glm::vec3, VertDatTy> *dat = nullptr;
        /// <summary>
        ///       /|\
        ///   010  |   011
        ///  110/  |/ 111/
        /// -------+------->
        ///  000  /|  001
        /// 100/ / | 101/
        ///    |/_
        /// </summary>
        std::array<Node *, 8> children{nullptr};

        Node(const glm::vec3 &min, const glm::vec3 &max) : min(min), max(max) {
            dat = new std::pair<glm::vec3, VertDatTy>[datCap];
        }
        ~Node() { Clear(); }
        template <typename Ty>
        inline void Insert(const glm::vec3 &pos, Ty &&vertDat) {
            if (datCap == 0) {
                datCap = DEFAULT_DAT_CAP;
                dat = new std::pair<glm::vec3, VertDatTy>[datCap];
            } else if (datNum == datCap) {
                datCap = datCap << 1;
                auto *newPtr = new std::pair<glm::vec3, VertDatTy>[datCap];
                memcpy(newPtr, dat,
                       sizeof(std::pair<glm::vec3, VertDatTy>) * datNum);
                delete[] dat;
                dat = newPtr;
            }
            dat[datNum].first = pos;
            dat[datNum++].second = std::forward<Ty>(vertDat);
        }
        inline void Clear() {
            if (datCap != 0) {
                delete[] dat;
                datNum = datCap = 0;
            }
        }
    };

  private:
    using NodeTy = Node<VertDatTy>;
    uint8_t maxDatNum = 8;
    bool rootIsLeaf = true;
    NodeTy root;

  public:
    PointOctree(const glm::vec3 &min, const glm::vec3 &max,
                uint8_t maxDatNum = 8)
        : root(min, max), maxDatNum(maxDatNum) {
        assert(maxDatNum > 1);
    }
    ~PointOctree() {
        std::stack<std::pair<NodeTy *, uint8_t>> stk;
        stk.emplace(&root, 0);
        while (!stk.empty()) {
            auto &[par, idx] = stk.top();
            if (idx == 8) {
                stk.pop();
                if (par != &root)
                    delete par;
                continue;
            }
            if (par->children[idx]) {
                NodeTy *curr = par->children[idx];
                stk.emplace(curr, 0);
                uint8_t *parIdxPtr = &stk.top().second;
                while (curr->children[0]) {
                    ++(*parIdxPtr);
                    curr = curr->children[0];
                    stk.emplace(curr, 0);
                    parIdxPtr = &stk.top().second;
                }
            }
            ++idx;
        }
    }
    std::pair<const NodeTy *, uint8_t>
    Query(const glm::vec3 &pos,
          float maxSqrErr = std::numeric_limits<float>::epsilon()) const {
        if (isOutOfBound(pos))
            return {nullptr, 0};
        const NodeTy *curr = &root;
        while (curr) {
            if (curr->datNum != 0) {
                // leaf node
                float minSqrErr = std::numeric_limits<float>::max();
                uint8_t minSqrErrIdx = 9;
                for (uint8_t datIdx = 0; datIdx < maxDatNum; ++datIdx) {
                    float sqrErr = 0;
                    for (uint8_t xyz = 0; xyz < 3; ++xyz)
                        sqrErr += (curr->dat[datIdx].first[xyz] - pos[xyz]) *
                                  (curr->dat[datIdx].first[xyz] - pos[xyz]);
                    if (sqrErr <= maxSqrErr && sqrErr < minSqrErr) {
                        minSqrErr = sqrErr;
                        minSqrErrIdx = datIdx;
                    }
                }
                if (minSqrErrIdx == 9)
                    return {nullptr, 0};
                return {curr, minSqrErrIdx};
            }
            curr =
                curr->children[getChildIdx((curr->min + curr->max) * .5f, pos)];
        }
        return {nullptr, 0};
    }
    std::vector<const NodeTy *> Query(const Frustum &frustum) {
        std::vector<const NodeTy *> ret;
        std::stack<NodeTy *> stk;
        stk.emplace(&root);
        while (!stk.empty()) {
            auto curr = stk.top();
            stk.pop();
            if (!frustum.IsIntersectedWithAABB(curr->min, curr->max))
                continue;
            if (curr->datNum == 0) {
                for (uint8_t chIdx = 0; chIdx < 8; ++chIdx)
                    if (curr->children[chIdx])
                        stk.emplace(curr->children[chIdx]);
            } else
                ret.emplace_back(curr);
        }
        return ret;
    }
    template <typename Ty>
    void Insert(const glm::vec3 &pos, Ty &&vertDat,
                float maxSqrErr = std::numeric_limits<float>::epsilon()) {
        if (isOutOfBound(pos))
            return;
        if (Query(pos, maxSqrErr).first)
            return; // duplicate
        if (rootIsLeaf)
            if (root.datNum == maxDatNum)
                rootIsLeaf = false;
            // then split root at successed procedure
            else {
                root.Insert(pos, std::forward<Ty>(vertDat));
                return;
            }
        NodeTy *curr = &root;
        while (true) {
            if (curr->datNum != 0) {
                // leaf node
                if (curr->datNum == maxDatNum) {
                    // spilit
                    NodeTy *oldNode = curr;
                    glm::vec3 mid;
                    while (true) {
                        mid = (curr->min + curr->max) * .5f;
                        uint8_t prevChIdx =
                            getChildIdx(mid, curr->dat[0].first);
                        uint8_t datIdx = 1;
                        for (; datIdx < maxDatNum; ++datIdx) {
                            auto chIdx =
                                getChildIdx(mid, oldNode->dat[datIdx].first);
                            if (chIdx != prevChIdx)
                                break;
                        }
                        if (datIdx != maxDatNum)
                            break; // can be spilited at current layer
                        // else insert a new layer
                        auto [min, max] = getChildMinAndMax(
                            curr->min, curr->max, mid, prevChIdx);
                        curr->children[prevChIdx] = new NodeTy(min, max);
                        curr = curr->children[prevChIdx];
                    }
                    mid = (curr->min + curr->max) * .5f;
                    for (uint8_t datIdx = 0; datIdx < maxDatNum; ++datIdx) {
                        auto chIdx =
                            getChildIdx(mid, oldNode->dat[datIdx].first);
                        if (!curr->children[chIdx]) {
                            auto [min, max] = getChildMinAndMax(
                                curr->min, curr->max, mid, chIdx);
                            curr->children[chIdx] = new NodeTy(min, max);
                        }
                        curr->children[chIdx]->Insert(
                            oldNode->dat[datIdx].first,
                            oldNode->dat[datIdx].second);
                    }
                    oldNode->Clear();
                    auto chIdx = getChildIdx(mid, pos);
                    if (!curr->children[chIdx]) {
                        auto [min, max] =
                            getChildMinAndMax(curr->min, curr->max, mid, chIdx);
                        curr->children[chIdx] = new NodeTy(min, max);
                    }
                    curr->children[chIdx]->Insert(pos,
                                                  std::forward<Ty>(vertDat));
                } else
                    curr->Insert(pos, std::forward<Ty>(vertDat));
                return;
            } else {
                // non-leaf node
                auto mid = (curr->min + curr->max) * .5f;
                auto chIdx = getChildIdx(mid, pos);
                if (!curr->children[chIdx]) {
                    auto [min, max] =
                        getChildMinAndMax(curr->min, curr->max, mid, chIdx);
                    curr->children[chIdx] = new NodeTy(min, max);
                    curr->children[chIdx]->Insert(pos, vertDat);
                    return;
                }
                curr = curr->children[chIdx];
            }
        }
    }
    friend std::ostream &operator<<(std::ostream &os,
                                    const PointOctree<VertDatTy> &tree) {
        std::queue<std::pair<decltype(&tree.root), uint8_t>> que;
        que.emplace(&tree.root, (uint8_t)0);
        size_t count = 0, currLayerNum = 1, currLayerAcc = 1;
        while (!que.empty()) {
            auto curr = que.front();
            que.pop();
            if (!curr.first)
                os << " |_" << (uint32_t)curr.second << "_| ";
            else if (curr.first->datNum != 0) {
                os << "|<" << (uint32_t)curr.second << ">("
                   << curr.first->dat[0].first.x << ','
                   << curr.first->dat[0].first.y << ','
                   << curr.first->dat[0].first.z
                   << "):" << curr.first->dat[0].second;
                for (uint8_t datIdx = 1; datIdx < curr.first->datNum; ++datIdx)
                    os << "; " << '(' << curr.first->dat[datIdx].first.x << ','
                       << curr.first->dat[datIdx].first.y << ','
                       << curr.first->dat[datIdx].first.z
                       << "):" << curr.first->dat[datIdx].second;
                os << "| ";
            } else {
                os << " |[" << (uint32_t)curr.second << "]| ";
                for (uint8_t chIdx = 0; chIdx < 8; ++chIdx)
                    que.emplace(curr.first->children[chIdx], chIdx);
            }
            if (curr.second == 7)
                os << " == ";

            ++count;
            if (count == currLayerAcc) {
                os << std::endl;
                currLayerNum <<= 3;
                currLayerAcc += currLayerNum;
            }
        }
        return os;
    }

  private:
    inline bool isOutOfBound(const glm::vec3 &pos) const {
        for (uint8_t xyz = 0; xyz < 3; ++xyz)
            if (pos[xyz] < root.min[xyz] && pos[xyz] >= root.max[xyz])
                return true;
        return false;
    }
    static inline uint8_t getChildIdx(const glm::vec3 &mid,
                                      const glm::vec3 &pos) {
        uint8_t idx = 0;
        for (uint8_t xyz = 0; xyz < 3; ++xyz)
            if (pos[xyz] < mid[xyz])
                ;
            else
                idx |= (0x1 << (2 - xyz));
        return idx;
    }
    static inline std::pair<glm::vec3, glm::vec3>
    getChildMinAndMax(const glm::vec3 &min, const glm::vec3 &max,
                      const glm::vec3 &mid, uint8_t chIdx) {
        std::pair<glm::vec3, glm::vec3> ret;
        for (uint8_t xyz = 0; xyz < 3; ++xyz)
            if ((chIdx & (0x1 << (2 - xyz))) != 0) {
                ret.first[xyz] = mid[xyz];
                ret.second[xyz] = max[xyz];
            } else {
                ret.first[xyz] = min[xyz];
                ret.second[xyz] = mid[xyz];
            }
        return ret;
    }
};
} // namespace kouek
#endif // !KOUEK_POINT_OCTREE_H
