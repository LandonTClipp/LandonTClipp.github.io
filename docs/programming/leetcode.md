---
title: Leetcode
---

## Array/String

### [Merged Sorted Array](https://leetcode.com/problems/merge-sorted-array/) (easy)

#### Problem Statement

You are given two integer arrays nums1 and nums2, sorted in non-decreasing order, and two integers m and n, representing the number of elements in nums1 and nums2 respectively.

Merge nums1 and nums2 into a single array sorted in non-decreasing order.

The final sorted array should not be returned by the function, but instead be stored inside the array nums1. To accommodate this, nums1 has a length of m + n, where the first m elements denote the elements that should be merged, and the last n elements are set to 0 and should be ignored. nums2 has a length of n.

 

Example 1:

    Input: nums1 = [1,2,3,0,0,0], m = 3, nums2 = [2,5,6], n = 3
    Output: [1,2,2,3,5,6]
    Explanation: The arrays we are merging are [1,2,3] and [2,5,6].
    The result of the merge is [1,2,2,3,5,6] with the underlined elements coming from nums1.

Example 2:

    Input: nums1 = [1], m = 1, nums2 = [], n = 0
    Output: [1]
    Explanation: The arrays we are merging are [1] and [].
    The result of the merge is [1].

Example 3:

    Input: nums1 = [0], m = 0, nums2 = [1], n = 1
    Output: [1]
    Explanation: The arrays we are merging are [] and [1].
    The result of the merge is [1].

 Note that because m = 0, there are no elements in nums1. The 0 is only there to ensure the merge result can fit in nums1.
 

Constraints:

- nums1.length == m + n
- nums2.length == n
- 0 <= m, n <= 200
- 1 <= m + n <= 200
- -109 <= nums1[i], nums2[j] <= 109
 

Follow up: Can you come up with an algorithm that runs in O(m + n) time?

#### [Solution](https://leetcode.com/problems/merge-sorted-array/submissions/1120616479)

##### Intuition
Because we know that both arrays come pre-sorted, we can take advantage of the fact that `i+1` will always be >= `i` for each array.

##### Approach
I decide to use a cursor-based approach. We will allocate a new slice of size `m+n` and use two individual cursors that point into each array respectively. Whenever we "consume" an element from either array, we'll increment the corresponding cursor. We'll need to account for edge cases where we have consumed all available values in a particular array.

##### Complexity
- Time complexity: $O(m+n)$

- Space complexity: $O(m+n)$

##### Code
```go
func merge(nums1 []int, m int, nums2 []int, n int)  {
    merged := make([]int, m+n)
    m_cursor := 0
    n_cursor := 0
    for i := 0; i < m+n; i++ {
        if m_cursor >= m {
            merged[i] = nums2[n_cursor]
            n_cursor++
            continue
        }
        if n_cursor >= n {
            merged[i] = nums1[m_cursor]
            m_cursor++
            continue
        }


        // Asumption in this block is that both m_cursor < m and
        // n_cursor < n. So we need to account for the cases where
        // we've consumed all the values from one or the other
        if nums1[m_cursor] < nums2[n_cursor] {
            merged[i] = nums1[m_cursor]
            m_cursor++
        } else if nums2[n_cursor] < nums1[m_cursor] {
            merged[i] = nums2[n_cursor]
            n_cursor++
        } else {
            // they must be equal, so pick an element arbitrarily
            merged[i] = nums1[m_cursor]
            m_cursor++
        }
    }
    for i := 0; i < m+n; i++ {
        nums1[i] = merged[i]
    }
}

```

## Two Pointers

### [Valid Palindrome](https://leetcode.com/problems/valid-palindrome/) (easy)

#### Problem Statement

A phrase is a palindrome if, after converting all uppercase letters into lowercase letters and removing all non-alphanumeric characters, it reads the same forward and backward. Alphanumeric characters include letters and numbers.

Given a string s, return true if it is a palindrome, or false otherwise.

 

Example 1:

    Input: s = "A man, a plan, a canal: Panama"
    Output: true
    Explanation: "amanaplanacanalpanama" is a palindrome.

Example 2:

    Input: s = "race a car"
    Output: false
    Explanation: "raceacar" is not a palindrome.

Example 3:

    Input: s = " "
    Output: true
    Explanation: s is an empty string "" after removing non-alphanumeric characters.
    Since an empty string reads the same forward and backward, it is a palindrome.
 

Constraints:

1. 1 <= s.length <= 2 * 105
2. s consists only of printable ASCII characters.

#### [Solution](https://leetcode.com/problems/valid-palindrome/submissions/1120632597)
##### Intuition

This is a fairly simple problem. We need to normalize the input string to disregard non-alphanumeric characters. There are a few ways you can do this, but I intend to use an approach that utilizes the least amount of data copying.

##### Approach

A naive approach would be to normalize the input string by copying each alphanumeric element into a new string and setting it to its lowercase representation. Then you could iterate over the normalized string and compare it to the opposing end of the string. However, this approach is costly as it requires lots of data copying and computation.

Instead, I used a cursor approach where we iterate over each element of the string. We will `continue` the loop if we found a non-alphanumeric character. Additionally, we keep track of the "opposing index" of the string, what we'll call `oppositeCursor`. In the `for` loop, the `oppositeCursor` is decremented until we find an alphanumeric character. Once that is found, we compare the lower-case representation at `s[i]` and `s[oppositeCursor]` and if they don't match, then it is not a valid palindrome.

#####Complexity
- Time complexity: $O(n)$

- Space complexity: $O(1)$

##### Code

```go
func isPalindrome(s string) bool {
    oppositeCursor := len(s)-1
    for i := 0; i < len(s); i++ {
        if i > oppositeCursor {
            break
        }
        if !isAlphaNumeric(s[i]) {
            continue
        }
        for;!isAlphaNumeric(s[oppositeCursor]) && i<oppositeCursor ; oppositeCursor-- {}
        if i > oppositeCursor || strings.ToLower(string(s[i])) != strings.ToLower(string(s[oppositeCursor])) {
            return false
        }
        oppositeCursor--
    }
    return true
}

func isAlphaNumeric(c byte) bool {
    r := rune(c)
    return unicode.IsLetter(r) || unicode.IsNumber(r)
}

```

## Sliding Window

### Minimum Size Subarray Sum (Medium)

#### Problem Statement

Given an array of positive integers nums and a positive integer target, return the minimal length of a 
subarray
 whose sum is greater than or equal to target. If there is no such subarray, return 0 instead.

 

Example 1:

    Input: target = 7, nums = [2,3,1,2,4,3]
    Output: 2
    Explanation: The subarray [4,3] has the minimal length under the problem constraint.

Example 2:

    Input: target = 4, nums = [1,4,4]
    Output: 1

Example 3:

    Input: target = 11, nums = [1,1,1,1,1,1,1,1]
    Output: 0
    

Constraints:

1 <= target <= 109
1 <= nums.length <= 105
1 <= nums[i] <= 104
 

Follow up: If you have figured out the O(n) solution, try coding another solution of which the time complexity is O(n log(n)).

#### Solution

[Naive (brute force):](https://leetcode.com/problems/minimum-size-subarray-sum/submissions/1122951997?envType=study-plan-v2&envId=top-interview-150)

```go
func minSubArrayLen(target int, nums []int) int {
    minLength := 0

outerloop:
    for left := 0; left < len(nums); left++ {
        cumulativeSum += left
        sum := 0

        for right := left; right < len(nums); right++ {
            sum += nums[right]
            if sum >= target {
                length := right - left + 1
                if minLength == 0 || minLength > length {
                    minLength = length
                }
                continue outerloop
            }
        }
    }
    return minLength
}
```

**Complexity Analysis**

- Time: $O(n^2)
  - The lefthand pointer of our subarray iterates over the entire `nums` array, which is $O(n)$.
  - For every position of `left`, we find every subarray starting from that point, which is $O(n)$.
  - Together, these operations are multiplied to become $O(n^2)$
- Space: $O(1)$

| status | language | runtime | memory |
|--------|----------|---------|--------|
| Accepted | Go | 2884ms | 8.4MB |

[Sliding Window:](https://leetcode.com/problems/minimum-size-subarray-sum/submissions/1122988496?envType=study-plan-v2&envId=top-interview-150)

```go
func minSubArrayLen(target int, nums []int) int {
    minLength := len(nums) + 1
    sum := 0
    left := 0
    for right := 0; right < len(nums); right++ {
        sum += nums[right]
        for sum >= target {
            minLength = min(minLength, right - left + 1)
            sum -= nums[left]
            left++
        }
    }

    if minLength == len(nums) + 1 {
        return 0
    }
    return minLength
}

```

| status | language | runtime | memory |
|--------|----------|---------|--------|
| Accepted | Go | 28ms | 7.8MB |

This solution uses a dynamically-sized array. We start with the smallest non-zero subarray at the lefthand side and increase the array until its total sum is greater than or equal to the target. Then, we increment the lefthand pointer until the sum is below the target again. During each iteration where we are incrementing the lefthand pointer, we set `minLength` equal to the current subarray length if it's smaller than the last recorded `minLength`.

The effect of this algorithm is that we only iterate over the entire subarray at most twice, which gives us $O(n)$.

## Matrix

### [Valid Sudoku](https://leetcode.com/problems/valid-sudoku/)

#### Problem Statement

Determine if a 9 x 9 Sudoku board is valid. Only the filled cells need to be validated according to the following rules:

1. Each row must contain the digits 1-9 without repetition.
2. Each column must contain the digits 1-9 without repetition.
3. Each of the nine 3 x 3 sub-boxes of the grid must contain the digits 1-9 without repetition.

Note:

A Sudoku board (partially filled) could be valid but is not necessarily solvable.
Only the filled cells need to be validated according to the mentioned rules.

#### Solution

There are a few ways this could be solved. A naive solution would be to first iterate over every `(x,y)` coordinate in the puzzle, and for each coordinate, traverse the entire column at `x` and the entire row at `y` to see if an integer repeats. This algoirthm would require $O(n^2)$ to iterate over every element, then $O(2n)=O(n)$ to iterate over the respective column/row, for a total time complexity of $O(n^3)$. The space complexity would be $O(n)$ as we would want to allocate a set that contains the values seen in the entire column/row for `(x,y)`.

Another solution would be as follows: 

1. Allocate two sets: one for every column, and one for every row. This gives a space complexity of $O(n^2)$.
2. Iterate over every `(x,y)` coordinate. This gives time complexity of $O(n^2)$
   a. If a value is present in the coordinate, add the value to the `cols[x]` set, and to the `cols[y]` set. Time complexity is $O(1)$ as hashing is constant time.
   b. If the value previously exists in either set, return `false`.
3. If we made it through the entire puzzle and a value was not repeated for each `(col,row)` tuple, then return `true`.

https://leetcode.com/problems/valid-sudoku/submissions/1124533660?envType=study-plan-v2&envId=top-interview-150

```go
func isValidSudoku(board [][]byte) bool {
    type set map[byte]struct{}
    type index int
    cols := make([]set, 9)
    rows := make([]set, 9)

    // The subBox that a particular coordinate belongs to is calculated using
    // the formula: subBox = floor(x/3) + (floor(y/3) * 3). The constraint says
    // that `board.length == 9` so we know there will always be 9 sub-boxes.
    subBox := make([]set, 9)

    boardLen := index(len(board))

    for y := index(0); y < boardLen; y++ {
        if rows[y] == nil {
            rows[y] = set{}
        }
        for x := index(0); x < boardLen;  x++ {
            // Initialize the cols[x] set if it is nil
            if cols[x] == nil {
                cols[x] = set{}
            }
            // Calculate the 3x3 sub-box that we're in. If it doesn't exist,
            // allocate it.
            subBoxIdx := (x/index(3)) + ((y/index(3)) * 3)
            subBoxElement := subBox[subBoxIdx]
            if subBoxElement == nil {
                subBoxElement = set{}
                subBox[subBoxIdx] = subBoxElement
            }

            val := board[x][y]
            if val == byte('.') {
                continue
            }
            // Has this number been seen in this row before?
            if _, existsInRow := rows[y][val]; existsInRow {
                return false
            }
            rows[y][val] = struct{}{}
            // Has this number been seen in this column before?
            if _, existsCol := cols[x][val]; existsCol {
                return false
            }
            cols[x][val] = struct{}{}
            // Has this number been seen in our subBox before?
            if _, existsInSubBox := subBox[subBoxIdx][val]; existsInSubBox {
                return false
            }
            subBox[subBoxIdx][val] = struct{}{}
        }
    }
    return true
}
```

| status | language | runtime | memory |
|--------|----------|---------|--------|
| Accepted | Go | 5ms (28.94%) | 3.4MB (35.58%) |

In this solution, our row/col/subBox hashmaps are stored inside an array of fixed length `9`. We could have used a structure like `map[index]set` but because we know beforehand the values that would go into the map, it's more efficient to instead use an array of length `9`: `[9]set`.

As you can see, this solution lies within the lower half percentile of all submissions in terms of runtime and memory performance. Perhaps there is a better way? I noticed one silly thing that was being done in my original solution: the initialization of the rows/copls/subBox sets were being done inside the main `for` loop. This means that there will be a lot of unnecessary branching done in the `if` statements that check if the set at a particular index had been initialized yet. Let's see how it performs with this change:

https://leetcode.com/problems/valid-sudoku/submissions/1124540277?envType=study-plan-v2&envId=top-interview-150

```go
type set map[byte]struct{}
type index int

func isValidSudoku(board [][]byte) bool {

    cols := make([]set, 9)
    rows := make([]set, 9)

    // The subBox that a particular coordinate belongs to is calculated using
    // the formula: subBox = floor(x/3) + (floor(y/3) * 3). The constraint says
    // that `board.length == 9` so we know there will always be 9 sub-boxes.
    subBox := make([]set, 9)

    boardLen := index(len(board))

    for i := index(0); i < boardLen; i++ {
        rows[i] = set{}
        cols[i] = set{}
        subBox[i] = set{}
    }

    for y := index(0); y < boardLen; y++ {
        for x := index(0); x < boardLen;  x++ {
            // Calculate the 3x3 sub-box that we're in. If it doesn't exist,
            // allocate it.
            subBoxIdx := (x/index(3)) + ((y/index(3)) * 3)

            val := board[x][y]
            if val == byte('.') {
                continue
            }
            // Has this number been seen in this row before?
            if _, existsInRow := rows[y][val]; existsInRow {
                return false
            }
            rows[y][val] = struct{}{}
            // Has this number been seen in this column before?
            if _, existsCol := cols[x][val]; existsCol {
                return false
            }
            cols[x][val] = struct{}{}
            // Has this number been seen in our subBox before?
            if _, existsInSubBox := subBox[subBoxIdx][val]; existsInSubBox {
                return false
            }
            subBox[subBoxIdx][val] = struct{}{}
        }
    }
    return true
}
```

| status | language | runtime | memory |
|--------|----------|---------|--------|
| Accepted | Go | 0ms (100.00%) | 3.56MB (31.71%) |

As we can see, the runtime is now _much_ better. However, being somewhat of a perfectionist, our memory consumption is still not where I'd like it to be. If you look at the `Memory` distribution for submissions, a large number of Leetcoders were able to get the usage down to ~2.6MB. Let's see what we can do to resolve this.

The two prior solutions are relying on a `#!go type set map[byte]struct{}` to represent a set of bytes that we've seen. However, one property in this particular problem is that we already know the maximum size that this set could ever be, which is 9 (due to the fact that Sudoku cells can only be from 1-9). When we know the size of the hashmap beforehand, we can instead use a fixed-length array. We'll set the value of the array to be a `bool`, which in Go is a single byte.

https://leetcode.com/problems/valid-sudoku/submissions/1124547528?envType=study-plan-v2&envId=top-interview-150

```go 
type index int

func isValidSudoku(board [][]byte) bool {
    cols := make([][9]bool, 9)
    rows := make([][9]bool, 9)

    // The subBox that a particular coordinate belongs to is calculated using
    // the formula: subBox = floor(x/3) + (floor(y/3) * 3). The constraint says
    // that `board.length == 9` so we know there will always be 9 sub-boxes.
    subBox := make([][9]bool, 9)

    boardLen := index(len(board))

    for i := index(0); i < boardLen; i++ {
        rows[i] = [9]bool{}
        cols[i] = [9]bool{}
        subBox[i] = [9]bool{}
    }

    for y := index(0); y < boardLen; y++ {
        for x := index(0); x < boardLen;  x++ {
            // Calculate the 3x3 sub-box that we're in. If it doesn't exist,
            // allocate it.
            subBoxIdx := (x/index(3)) + ((y/index(3)) * 3)

            val := board[x][y]
            if val == byte('.') {
                continue
            }
            valInt, _ := strconv.Atoi(string(val))
            valInt--
            // Has this number been seen in this row before?
            if rows[y][valInt] {
                return false
            }
            rows[y][valInt] = true
            // Has this number been seen in this column before?
            if cols[x][valInt] {
                return false
            }
            cols[x][valInt] = true
            // Has this number been seen in our subBox before?
            if subBox[subBoxIdx][valInt] {
                return false
            }
            subBox[subBoxIdx][valInt] = true
        }
    }
    return true
}
```

| status | language | runtime | memory |
|--------|----------|---------|--------|
| Accepted | Go | 0ms (100.00%) | 2.66MB (73.09%) |

There's a way to make this even more memory efficient. For each row/col/box, we're storing a single `bool` for each integer `1-9`. This totals to 9 bytes for each row/column/box. Ideally, we'd like to distill down our bool to a single bit, but there is no data type that is a single bit large. Instead, we could rely on bitmasking over a sufficiently sized data type (perhaps a uint16) to store our bool. Our data structures would look something like:

```go
cols := [9]uint16{}
rows := [9]uint16{}
subBox := [9]uint16{}
```

Notice that in this solution, I'll elect to use an array instead of a slice, as arrays take up even less memory in Go.

The bit mask is going to need to be equal to its location in the `uint16`, or more specifically, `#!go 1 << valInt`

https://leetcode.com/problems/valid-sudoku/submissions/1124554856?envType=study-plan-v2&envId=top-interview-150

```go
type index int

func isValidSudoku(board [][]byte) bool {
    cols := [9]uint16{}
    rows := [9]uint16{}
    subBox := [9]uint16{}
    valMap := map[byte]int{}

    boardLen := index(len(board))

    for y := index(0); y < boardLen; y++ {
        for x := index(0); x < boardLen;  x++ {
            // Calculate the 3x3 sub-box that we're in. If it doesn't exist,
            // allocate it.
            subBoxIdx := (x/index(3)) + ((y/index(3)) * 3)

            val := board[x][y]
            if val == byte('.') {
                continue
            }
            var valInt int
            var exists bool
            if valInt, exists = valMap[val]; !exists {
                v, _ := strconv.Atoi(string(val))
                v--
                valMap[val] = v
            }
            valInt = valMap[val]
           
            mask := (uint16(1)<<valInt)
            // Has this number been seen in this row before?
            if rows[y] & mask != 0 {
                return false
            }
            rows[y] |= mask
            // Has this number been seen in this column before?
            if cols[x] & mask != 0 {
                return false
            }
            cols[x] |= mask
            // Has this number been seen in our subBox before?
            if subBox[subBoxIdx] & mask != 0 {
                return false
            }
            subBox[subBoxIdx] |= mask
        }
    }
    return true
}
```

| status | language | runtime | memory |
|--------|----------|---------|--------|
| Accepted | Go | 0ms (100.00%) | 2.64MB (73.09%) |

While this solution is slightly more efficient with memory, it was barely noticable. For a `9x9` grid, we were able to shave off $(9+9+9)*(1*9) - (9+9+9)*1 = 216$ bytes (in the prior solution, each row/col/box required a single byte for each value 1-9, or 9 bytes total, but in this solution, each row/col/box needs only 2 bytes).
