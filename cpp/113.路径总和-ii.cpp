/*
 * @lc app=leetcode.cn id=113 lang=cpp
 *
 * [113] 路径总和 II
 */

// @lc code=start
/**
 * Definition for a binary tree node.
 * struct TreeNode {
 *     int val;
 *     TreeNode *left;
 *     TreeNode *right;
 *     TreeNode(int x) : val(x), left(NULL), right(NULL) {}
 * };
 */
class Solution {
public:
    vector<vector<int>> ans;
    vector<int> temp;
    vector<vector<int>> pathSum(TreeNode* root, int sum) {
        if(root == nullptr) return ans;
        temp.push_back(root->val);
        dfs(root, sum, root->val);
        return ans;
    }

    void dfs(TreeNode* root, int sum, int val) {
        if(root->left == nullptr && root->right == nullptr && val == sum) {
            ans.push_back(temp);
            return;
        } 
        if(root->left == nullptr && root->right == nullptr && val != sum) return;
        if(root->left != nullptr) {
            temp.push_back(root->left->val);
            dfs(root->left, sum, val+root->left->val);
            temp.pop_back();
        }
        if(root->right != nullptr) {
            temp.push_back(root->right->val);
            dfs(root->right, sum, val+root->right->val);
            temp.pop_back();
        }
        return;
    }
};
// @lc code=end

