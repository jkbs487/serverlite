//https://leetcode-cn.com/problems/container-with-most-water

class Solution {
public:
  int maxArea(vector<int>& height) {
      int maxAr = 0;
      for(int i = 0; i < height.size(); i++){
          for(int j = i+1; j < height.size(); j++){
             if(height[j] <= height[i]){
                 maxAr = max(maxAr, height[j] * (j-i));
             } 
              if(height[j] > height[i]){
                  maxAr = max(maxAr, height[i] * (j-i));
              }
          }
      }
      return maxAr;
  }
};
