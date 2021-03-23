// Source : https://leetcode-cn.com/problems/flatten-nested-list-iterator/
// Author : jkbs487
// Date   : 2021-03-23

/***************************************************************************************************** 
 *
 * Given a nested list of integers, implement an iterator to flatten it.
 * 
 * Each element is either an integer, or a list -- whose elements may also be integers or other lists.
 * 
 * Example 1:
 * 
 * Input: [[1,1],2,[1,1]]
 * Output: [1,1,2,1,1]
 * Explanation: By calling next repeatedly until hasNext returns false, 
 *              the order of elements returned by next should be: [1,1,2,1,1].
 * 
 * Example 2:
 * 
 * Input: [1,[4,[6]]]
 * Output: [1,4,6]
 * Explanation: By calling next repeatedly until hasNext returns false, 
 *              the order of elements returned by next should be: [1,4,6].
 * 
 ******************************************************************************************************/

/**
 * // This is the interface that allows for creating nested lists.
 * // You should not implement it, or speculate about its implementation
 * class NestedInteger {
 *   public:
 *     // Return true if this NestedInteger holds a single integer, rather than a nested list.
 *     bool isInteger() const;
 *
 *     // Return the single integer that this NestedInteger holds, if it holds a single integer
 *     // The result is undefined if this NestedInteger holds a nested list
 *     int getInteger() const;
 *
 *     // Return the nested list that this NestedInteger holds, if it holds a nested list
 *     // The result is undefined if this NestedInteger holds a single integer
 *     const vector<NestedInteger> &getList() const;
 * };
 */

//using dfs flatten vector
class NestedIterator {
public:
    NestedIterator(vector<NestedInteger> &nestedList){
        fullIntergerList(nestedList);
        iter = integerList.begin(); 
    }

    int next() {
        return *iter++;
    }
    
    bool hasNext() {
        return iter != integerList.end();
    }

private:
    vector<int> integerList;
    vector<int>::iterator iter;
    void fullIntergerList(vector<NestedInteger> &nestedList) {
        for (auto nested : nestedList) {
            if (nested.isInteger()) {
                integerList.push_back(nested.getInteger());
            }
            else {
                fullIntergerList(nested.getList());
            }
        }
        return;
    }
};

/**
 * Your NestedIterator object will be instantiated and called as such:
 * NestedIterator i(nestedList);
 * while (i.hasNext()) cout << i.next();
 */
