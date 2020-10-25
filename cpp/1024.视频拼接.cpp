/*
 * @lc app=leetcode.cn id=1024 lang=cpp
 *
 * [1024] 视频拼接
 */

// @lc code=start
class Solution {
public:
    int videoStitching(vector<vector<int>>& clips, int T) {
        int ans = 1;
        int len = clips.size();
        sort(clips.begin(), clips.end());
        if(clips[0][0] != 0) return -1;
        int temp = 0, maxNum = 0;
        for(int i = 0; i < len; i++) {
            if(clips[i][0] > temp) {
                temp = maxNum;
                ans++;
            }
            if(clips[i][0] <= temp) {
                maxNum = max(maxNum, clips[i][1]);
                if(maxNum >= T) break;
            }
        }
        return maxNum >= T ? ans : -1;
    }
};
// @lc code=end

