//https://leetcode-cn.com/problems/string-to-integer-atoi/

class Solution {
public:
    int myAtoi(string str) {
        int n = str.size(), atoi = 0, flag = 1;
        if(str.empty()) return 0;
        int i = str.find_first_not_of(' ');
        if(str[i] == '+' || str[i] == '-'){
            flag = (str[i++] == '+') ? 1 : -1;
        }
        while(i<n && str[i]<='9' && str[i]>='0'){
            if(atoi > (INT_MAX-str[i]+'0')/10){
                return (flag == 1) ? INT_MAX : INT_MIN;
            }
            atoi = str[i++]-'0'+(atoi*10);
        }
        return flag*atoi;
    }
};
