#include <string>
#include <vector>
#include <map>

size_t lev_edit_distance(size_t len1, const char* string1, size_t len2, const char* string2, int xcost) {
    size_t i;
    size_t* row; /* we only need to keep one row of costs */
    size_t* end;
    size_t half;

    /* strip common prefix */
    while (len1 > 0 && len2 > 0 && *string1 == *string2) {
        len1--;
        len2--;
        string1++;
        string2++;
    }

    /* strip common suffix */
    while (len1 > 0 && len2 > 0 && string1[len1 - 1] == string2[len2 - 1]) {
        len1--;
        len2--;
    }

    /* catch trivial cases */
    if (len1 == 0)
        return len2;
    if (len2 == 0)
        return len1;

    /* make the inner cycle (i.e. string2) the longer one */
    if (len1 > len2) {
        size_t nx = len1;
        const char* sx = string1;
        len1 = len2;
        len2 = nx;
        string1 = string2;
        string2 = sx;
    }
    /* check len1 == 1 separately */
    if (len1 == 1) {
        if (xcost)
            return len2 + 1 - 2 * (memchr(string2, *string1, len2) != NULL);
        else
            return len2 - (memchr(string2, *string1, len2) != NULL);
    }
    len1++;
    len2++;
    half = len1 >> 1;

    /* initalize first row */
    row = (size_t*)malloc(len2 * sizeof(size_t));
    if (!row)
        return (size_t)(-1);
    end = row + len2 - 1;
    for (i = 0; i < len2 - (xcost ? 0 : half); i++)
        row[i] = i;

    /* go through the matrix and compute the costs.  yes, this is an extremely
	* obfuscated version, but also extremely memory-conservative and relatively
	* fast.  */
    if (xcost) {
        for (i = 1; i < len1; i++) {
            size_t* p = row + 1;
            const char char1 = string1[i - 1];
            const char* char2p = string2;
            size_t D = i;
            size_t x = i;
            while (p <= end) {
                if (char1 == *(char2p++))
                    x = --D;
                else
                    x++;
                D = *p;
                D++;
                if (x > D)
                    x = D;
                *(p++) = x;
            }
        }
    } else {
        /* in this case we don't have to scan two corner triangles (of size len1/2)
		* in the matrix because no best path can go throught them. note this
		* breaks when len1 == len2 == 2 so the memchr() special case above is
		* necessary */
        row[0] = len1 - half - 1;
        for (i = 1; i < len1; i++) {
            size_t* p;
            const char char1 = string1[i - 1];
            const char* char2p;
            size_t D, x;
            /* skip the upper triangle */
            if (i >= len1 - half) {
                size_t offset = i - (len1 - half);
                size_t c3;

                char2p = string2 + offset;
                p = row + offset;
                c3 = *(p++) + (char1 != *(char2p++));
                x = *p;
                x++;
                D = x;
                if (x > c3)
                    x = c3;
                *(p++) = x;
            } else {
                p = row + 1;
                char2p = string2;
                D = x = i;
            }
            /* skip the lower triangle */
            if (i <= half + 1)
                end = row + len2 + i - half - 2;
            /* main */
            while (p <= end) {
                size_t c3 = --D + (char1 != *(char2p++));
                x++;
                if (x > c3)
                    x = c3;
                D = *p;
                D++;
                if (x > D)
                    x = D;
                *(p++) = x;
            }
            /* lower triangle sentinel */
            if (i <= half) {
                size_t c3 = --D + (char1 != *char2p);
                x++;
                if (x > c3)
                    x = c3;
                *p = x;
            }
        }
    }

    i = *end;
    free(row);
    return i;
}

class Triple {
private:
    int idx_1;
    int idx_2;
    int len;

public:
    Triple(int i, int j, int k) {
        idx_1 = i;
        idx_2 = j;
        len = k;
    }

    int& operator[](int idx) {
        switch (idx) {
        case 0:
            return idx_1;
        case 1:
            return idx_2;
        case 2:
            return len;
        }

        return idx_1;
    }

    bool operator==(const Triple& other) const {
        return this->idx_1 == other.idx_1 && this->idx_2 == other.idx_2 && this->len == other.len;
    }
};

//---------------------------------------------------------------------------

class SequenceMatcher {
private:
    std::string _str1, _str2;
    double _ratio, _distance;
    std::vector<Triple> _matching_blocks;
    std::map<std::pair<int, int>, int> _matchingBlocks;

    void _reset_cache(void) {
        _ratio = _distance = -1;
    }

    static int Levenshtein(const std::string& s1, const std::string& s2, int cost) {
        return lev_edit_distance(s1.length(), s1.c_str(), s2.length(), s2.c_str(), cost);
    }

public:
    SequenceMatcher(const std::string& str1, const std::string& str2) {
        _str1 = str1;
        _str2 = str2;
        _reset_cache();
    }

    virtual ~SequenceMatcher(void) {
    }

    double ratio(void) {
        if (_ratio == -1) {
            int lensum = _str1.length() + _str2.length();

            if (lensum == 0) {
                _ratio = 1.0;
            } else {
                int ldist = Levenshtein(_str1, _str2, 1);
                _ratio = (double)(lensum - ldist) / (double)lensum;
            }
        }

        return _ratio;
    }

    std::vector<Triple> get_matching_blocks(void) {
        _matching_blocks.clear();

        int str1_length = _str1.length();
        int str2_length = _str2.length();
        int str1_idx = 0;
        int str2_idx = -1;
        int buffer_length = 1;

        while (str1_idx + buffer_length - 1 < str1_length) {
            str2_idx = 0;
            while (str1_idx + buffer_length - 1 < str1_length) {
                std::string buffer_1 = _str1.substr(str1_idx, buffer_length);
                str2_idx = _str2.find(buffer_1, str2_idx);

                if (str2_idx != std::string::npos) {
                    _matching_blocks.push_back(Triple(str1_idx, str2_idx, buffer_length));
                    ++buffer_length;
                } else {
                    // Check & save last occurrence
                    ++str1_idx;
                    buffer_length = 1;
                    break;
                }
            }
        }

        Triple dummy(str1_length, str2_length, 0);
        _matching_blocks.push_back(dummy);

        return _matching_blocks;
    }

    std::map<std::pair<int, int>, int> GetMatchingBlocks(void) {
        _matchingBlocks.clear();

        int str1_length = _str1.length();
        int str2_length = _str2.length();
        int str1_idx = 0;
        int str2_idx = -1;
        int buffer_length = 1;

        while (str1_idx + buffer_length - 1 < str1_length) {
            str2_idx = 0;
            while (str1_idx + buffer_length - 1 < str1_length) {
                std::string buffer_1 = _str1.substr(str1_idx, buffer_length);
                str2_idx = _str2.find(buffer_1, str2_idx);

                if (str2_idx != std::string::npos) {
                    _matchingBlocks[std::make_pair(str1_idx, str2_idx)] = buffer_length;
                    ++buffer_length;
                } else {
                    // Check & save last occurrence
                    ++str1_idx;
                    buffer_length = 1;
                    break;
                }
            }
        }

        Triple dummy(str1_length, str2_length, 0);
        _matchingBlocks[std::make_pair(str1_length, str2_length)] = 0;

        return _matchingBlocks;
    }
};

double ratio(const std::string& s1, const std::string& s2) {
    SequenceMatcher m(s1, s2);
    return 100.0 * m.ratio();
}

double partial_ratio(const std::string& s1, const std::string& s2) {
    std::string shorter, longer;

    if (s1.length() <= s2.length()) {
        shorter = s1;
        longer = s2;
    } else {
        shorter = s2;
        longer = s1;
    }

    // 		printf_s("shorter = %s\n", shorter.c_str());
    // 		printf_s("longer  = %s\n", longer.c_str());
    SequenceMatcher m(shorter, longer);

    //const std::vector<Triple>& blocks = m.get_matching_blocks();

    const std::map<std::pair<int, int>, int>& blocks1 = m.GetMatchingBlocks();

    /* each block represents a sequence of matching characters in a string
	* of the form (idx_1, idx_2, len)
	* the best partial match will block align with at least one of those blocks
	*   e.g. shorter = "abcd", longer = XXXbcdeEEE
	*   block = (1,3,3)
	*   best score === ratio("abcd", "Xbcd")
	*/
    double max = -1.0;
    int str1_idx;
    int str2_idx;
    int len;

    for (auto& it : blocks1) {
        str1_idx = it.first.first;
        str2_idx = it.first.second;
        len = it.second;

        int long_start = (str2_idx - str1_idx > 0) ? str2_idx - str1_idx : 0;
        int long_end = long_start + shorter.length();
        std::string long_substr = longer.substr(long_start, long_end - long_start);

        //printf_s("idx_1 = %2d, idx_2 = %3d, len = %d\n", str1_idx, str2_idx, len);
        // 		printf_s("long_substr = %s\n", long_substr.c_str());
        SequenceMatcher m2(shorter, long_substr);

        double r = m2.ratio();

        if (r > 0.995) {
            return 100.0;
        } else if (r > max || max < 0) {
            max = r;
        }
    }

    return max * 100.0;
}
