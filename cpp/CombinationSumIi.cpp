// Source : https://leetcode-cn.com/problems/combination-sum-ii/
// Author : jkbs487
// Date   : 2020-12-16

/***************************************************************************************************** 
 *
 * Given a collection of candidate numbers (candidates) and a target number (target), find all unique 
 * combinations in candidates where the candidate numbers sum to target.
 * 
 * Each number in candidates may only be used once in the combination.
 * 
 * Note: The solution set must not contain duplicate combinations.
 * 
 * Example 1:
 * 
 * Input: candidates = [10,1,2,7,6,1,5], target = 8
 * Output: 
 * [
 * [1,1,6],
 * [1,2,5],
 * [1,7],
 * [2,6]
 * ]
 * 
 * Example 2:
 * 
 * Input: candidates = [2,5,2,1,2], target = 5
 * Output: 
 * [
 * [1,2,2],
 * [5]
 * ]
 * 
 * Constraints:
 * 
 * 	1 <= candidates.length <= 100
 * 	1 <= candidates[i] <= 50
 * 	1 <= target <= 30
 ******************************************************************************************************/

class Solution {
public:
    vector<vector<int>> combinationSum2(vector<int>& candidates, int target) {
        vector<vector<int>> ans;
        sort(candidates.begin(), candidates.end());
        dfs(ans, candidates, {}, target, 0);
        return ans;
    }

    void dfs(vector<vector<int>>& ans, vector<int>& candidates, vector<int> temp, int target, int index) {
        if(target == 0) {
            ans.push_back(temp);
            return;
        }
        for(int i = index; i < candidates.size() && target - candidates[i] >= 0; ++i) {
            if(i > index && candidates[i] == candidates[i-1])
                continue;
            temp.push_back(candidates[i]);
            dfs(ans, candidates, temp, target-candidates[i], i+1);
            res.pop_back();
        }
        return;
    }
};
