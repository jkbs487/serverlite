//https://leetcode-cn.com/problems/subsets-ii/

class Solution {
    vector<vector<int>> res;
    vector<int> temp;
public:
    vector<vector<int>> subsetsWithDup(vector<int>& nums) {
        sort(nums.begin(), nums.end());
        recur(nums, 0);
        return res;
    }
    void recur(vector<int>& nums, int index){
        res.push_back(temp);
        for(int i = index; i < nums.size(); i++){
            if(i > index && nums[i] == nums[i-1]) continue;	//去掉该层相同的数字
            temp.push_back(nums[i]);
            recur(nums, i + 1);
            temp.pop_back();
        }
        return;
    }
};
