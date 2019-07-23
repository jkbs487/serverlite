//https://leetcode-cn.com/problems/zigzag-conversion
class Solution {
public:
    string convert(string s, int numRows) {
        vector<string> temp(numRows);
        string res;
        if(s.empty() || numRows < 1) return res;
        if(numRows == 1) return s;
        for(int i = 0; i < s.size(); i++){
            int ans = i / (numRows-1);
            int remain = i % (numRows-1);
            if(ans % 2 == 0){                           //结果为偶数或0
                temp[remain].push_back(s[i]);              //按余数正序保存
            }
            if(ans % 2 != 0){                           //结果为奇数
                temp[numRows-remain-1].push_back(s[i]);    //按余数倒序保存
            }
        }
       for(int i = 0; i < temp.size(); i++){
               res += temp[i];
        }
        return res;
    }
};
