/*
 * @lc app=leetcode.cn id=925 lang=cpp
 *
 * [925] 长按键入
 */

// @lc code=start
class Solution {
public:
    bool isLongPressedName(string name, string typed) {
        int i = 0, j = 0;
        int name_len = name.size(), typed_len = typed.size();
        while(j < typed_len) {
            if(i < name_len && name[i] == typed[j]) {
                j++;
                i++;
            }
            else if(j > 0 && typed[j] == typed[j-1]) {
                j++;
            }
            else return false;
        }
        return i == name_len;
    }
};
// @lc code=end

