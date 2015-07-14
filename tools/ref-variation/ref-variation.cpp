/*===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's official duties as a United States Government employee and
*  thus cannot be copyrighted.  This software/database is freely available
*  to the public for use. The National Library of Medicine and the U.S.
*  Government have not placed any restriction on its use or reproduction.
*
*  Although all reasonable efforts have been taken to ensure the accuracy
*  and reliability of the software and data, the NLM and the U.S.
*  Government do not and cannot warrant the performance or results that
*  may be obtained by using this software or data. The NLM and the U.S.
*  Government disclaim all warranties, express or implied, including
*  warranties of performance, merchantability or fitness for any particular
*  purpose.
*
*  Please cite the author in any work or product based on this material.
*
* ===========================================================================
*
*/

#include "ref-variation.vers.h"
#include "helper.h"

#include <iostream>
#include <string.h>
#include <vector>
#include <string>
#include <stdio.h>
#include <stdint.h>

#ifdef _WIN32
#include <intrin.h>
#ifndef __builtin_popcount
#define __builtin_popcount __popcnt
#endif
#endif

#include <kapp/main.h>
#include <klib/rc.h>

#ifndef min
#define min(x,y) ((y) < (x) ? (y) : (x))
#endif

#ifndef max
#define max(x,y) ((y) >= (x) ? (y) : (x))
#endif

#define max4(x1, x2, x3, x4) (max( max((x1),(x2)), max((x3),(x4)) ))

namespace RefVariation
{
    struct Params
    {
        // command line params

        // command line options
        char const* ref_acc;
        int64_t ref_pos_var;
        char const* query;
        int verbosity;

    } g_Params =
    {
        "",
        -1,
        "",
        0
    };
    char const OPTION_REFERENCE_ACC[] = "reference-accession";
    char const ALIAS_REFERENCE_ACC[]  = "r";
    char const* USAGE_REFERENCE_ACC[] = { "look for the variation in this reference", NULL };

    char const OPTION_REF_POS[] = "position";
    char const ALIAS_REF_POS[]  = "p";
    char const* USAGE_REF_POS[] = { "look for the variation at this position on the reference", NULL };

    char const OPTION_QUERY[] = "query";
    char const ALIAS_QUERY[]  = "q";
    char const* USAGE_QUERY[] = { "query to find in the given reference", NULL };

    char const OPTION_VERBOSITY[] = "verbose";
    char const ALIAS_VERBOSITY[]  = "v";
    char const* USAGE_VERBOSITY[] = { "increase the verbosity of the program. use multiple times for more verbosity.", NULL };

    ::OptDef Options[] =
    {
        { OPTION_REFERENCE_ACC, ALIAS_REFERENCE_ACC, NULL, USAGE_REFERENCE_ACC, 1, true, true },
        { OPTION_REF_POS,       ALIAS_REF_POS,       NULL, USAGE_REF_POS,       1, true, true },
        { OPTION_QUERY,         ALIAS_QUERY,     NULL, USAGE_QUERY,         1, true, true }
        //{ OPTION_VERBOSITY,     ALIAS_VERBOSITY,     NULL, USAGE_VERBOSITY,     0, false, false }
    };

    unsigned char const map_char_to_4na [256] =
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 1,14, 2,13, 0, 0, 4,11, 0, 0,12, 0, 3,15, 0,
        0, 0, 5, 6, 8, 0, 7, 9, 0,10, 0, 0, 0, 0, 0, 0,
        0, 1,14, 2,13, 0, 0, 4,11, 0, 0,12, 0, 3,15, 0,
        0, 0, 5, 6, 8, 0, 7, 9, 0,10, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    int compare_4na ( char ch2na, char ch4na )
    {
        unsigned char bits4na = map_char_to_4na [(int)ch4na];
        unsigned char bits2na = map_char_to_4na [(int)ch2na];

        //return (bits2na & bits4na) != 0 ? 2 : -1;

        unsigned char popcnt4na;
        popcnt4na = (unsigned char) __builtin_popcount ( bits4na );

        return (bits2na & bits4na) != 0 ? 12 / popcnt4na : -6;
    }

    #define COMPARE_4NA 0

    int similarity_func (char ch2na, char ch4na)
    {
    #if COMPARE_4NA == 1
        return compare_4na ( ch2na, ch4na );
    #else
        return ch2na == ch4na ? 2 : -1;
    #endif
    }

    int gap_score_func ( size_t idx )
    {
    #if COMPARE_4NA == 1
        return -6*(int)idx;
    #else
        return -(int)idx;
    #endif
    }

    #define CACHE_MAX_COLS 1
    #define CACHE_MAX_ROWS 1

    template <bool t_reverse> class CStringIterator
    {
        char const* m_arr;
        size_t m_size;
    public:
        CStringIterator (char const* arr, size_t size)
            : m_arr(arr), m_size(size)
        {
        }

        char const& operator[] (size_t i) const;
        size_t get_straight_index (size_t logical_index);
    };

    template <> char const& CStringIterator<false>::operator[] (size_t i) const
    { return m_arr [i]; }

    template <> char const& CStringIterator<true>::operator[] (size_t i) const
    { return m_arr [m_size - i - 1]; }

    template <> size_t CStringIterator<false>::get_straight_index (size_t logical_index)
    { return logical_index; }

    template <> size_t CStringIterator<true>::get_straight_index (size_t logical_index)
    { return m_size - logical_index - 1; }


    template <bool reverse> void calculate_similarity_matrix (
        char const* text, size_t size_text,
        char const* query, size_t size_query,
        int* matrix)
    {
        size_t ROWS = size_text + 1;
        size_t COLUMNS = size_query + 1;

        // init 1st row and column with zeros
        memset ( matrix, 0, COLUMNS * sizeof(matrix[0]) );
        for ( size_t i = 1; i < ROWS; ++i )
            matrix [i * COLUMNS] = 0;

        // arrays to store maximums for all previous rows and columns
#ifdef CACHE_MAX_COLS
        typedef std::pair<int, size_t> TMaxPos;
        std::vector<TMaxPos> vec_max_cols(COLUMNS, TMaxPos(0, 0));
        std::vector<TMaxPos> vec_max_rows(ROWS, TMaxPos(0, 0));
#endif

        CStringIterator<reverse> text_iterator(text, size_text);
        CStringIterator<reverse> query_iterator(query, size_query);

        for ( size_t i = 1; i < ROWS; ++i )
        {
            for ( size_t j = 1; j < COLUMNS; ++j )
            {
                int sim = similarity_func ( text_iterator[i-1], query_iterator[j-1] );

#ifdef CACHE_MAX_COLS
                int cur_score_del = vec_max_cols[j].first + gap_score_func(j - vec_max_cols[j].second);
#else
                int cur_score_del = -1;
                for ( size_t k = 1; k < i; ++k )
                {
                    int cur = matrix [ (i - k)*COLUMNS + j ] + gap_score_func(k);
                    if ( cur > cur_score_del )
                        cur_score_del = cur;
                }
#endif

#ifdef CACHE_MAX_ROWS
                int cur_score_ins = vec_max_rows[i].first + gap_score_func(i - vec_max_rows[i].second);;
#else
                int cur_score_ins = -1;
                for ( size_t l = 1; l < j; ++l )
                {
                    int cur = matrix [ i*COLUMNS + (j - l) ] + gap_score_func(l);
                    if ( cur > cur_score_ins )
                        cur_score_ins = cur;
                }
#endif

                matrix[i*COLUMNS + j] = max4 (0,
                                              matrix[(i-1)*COLUMNS + j - 1] + sim,
                                              cur_score_del,
                                              cur_score_ins);

#ifdef CACHE_MAX_COLS
                if ( matrix[i*COLUMNS + j] > vec_max_cols[j].first )
                    vec_max_cols[j] = TMaxPos(matrix[i*COLUMNS + j], j);

                vec_max_cols[j].first += gap_score_func(1);
#endif
#ifdef CACHE_MAX_ROWS
                if ( matrix[i*COLUMNS + j] > vec_max_rows[i].first )
                    vec_max_rows[i] = TMaxPos(matrix[i*COLUMNS + j], i);

                vec_max_rows[i].first += gap_score_func(1);
#endif

            }
        }
    }

    void sw_find_indel_box ( int* matrix, size_t ROWS, size_t COLUMNS,
                             int* ret_row_start, int* ret_row_end,
                             int* ret_col_start, int* ret_col_end )
    {
        // find maximum score in the matrix
        size_t max_row = 0, max_col = 0;
        size_t max_i = 0;

        size_t i = ROWS*COLUMNS - 1;
        //do
        //{
        //    if ( matrix[i] > matrix[max_i] )
        //        max_i = i;
        //    --i;
        //}
        //while (i > 0);

        // TODO: prove the lemma: for all i: matrix[i] <= matrix[ROWS*COLUMNS - 1]
        // (i.e. matrix[ROWS*COLUMNS - 1] is always the maximum element in the valid SW-matrix)

        max_i = ROWS*COLUMNS - 1;

        max_row = max_i / COLUMNS;
        max_col = max_i % COLUMNS;


        // traceback to (0,0)-th element of the matrix
        *ret_row_start = *ret_row_end = *ret_col_start = *ret_col_end = -1;

        i = max_row;
        size_t j = max_col;
        bool prev_indel = false;
        while (true)
        {
            if (i > 0 && j > 0)
            {
                if ( matrix [(i - 1)*COLUMNS + (j - 1)] >= matrix [i*COLUMNS + (j - 1)] &&
                     matrix [(i - 1)*COLUMNS + (j - 1)] >= matrix [(i - 1)*COLUMNS + j])
                {
                    --i;
                    --j;

                    if (prev_indel)
                    {
                        *ret_row_start = (int)i;
                        *ret_col_start = (int)j;
                    }
                    prev_indel = false;
                }
                else if ( matrix [(i - 1)*COLUMNS + (j - 1)] < matrix [i*COLUMNS + (j - 1)] )
                {
                    if ( *ret_row_end == -1 )
                    {
                        *ret_row_end = (int)i;
                        *ret_col_end = (int)j;
                    }
                    --j;
                    prev_indel = true;
                }
                else
                {
                    if ( *ret_row_end == -1 )
                    {
                        *ret_row_end = (int)i;
                        *ret_col_end = (int)j;
                    }
                    --i;
                    prev_indel = true;
                }
            }
            else if ( i > 0 )
            {
                if ( *ret_row_end == -1 )
                {
                    *ret_row_end = (int)i;
                    *ret_col_end = 0;
                }
                *ret_row_start = 0;
                *ret_col_start = 0;
                break;
            }
            else if ( j > 0 )
            {
                if ( *ret_row_end == -1 )
                {
                    *ret_row_end = 0;
                    *ret_col_end = (int)j;
                }
                *ret_row_start = 0;
                *ret_col_start = 0;
                break;
            }
            else
            {
                break;
            }
        }
    }
    template <bool reverse> void print_matrix ( int const* matrix,
                                                char const* ref_slice, size_t ref_slice_size,
                                                char const* query, size_t query_size)
    {
        size_t COLUMNS = ref_slice_size + 1;
        size_t ROWS = query_size + 1;

        int print_width = 2;

        CStringIterator<reverse> ref_slice_iterator(ref_slice, ref_slice_size);
        CStringIterator<reverse> query_iterator(query, query_size);

        printf ("  %*c ", print_width, '-');
        for (size_t j = 0; j < COLUMNS; ++j)
            printf ("%*c ", print_width, ref_slice_iterator[j]);
        printf ("\n");

        for (size_t i = 0; i < ROWS; ++i)
        {
            if ( i == 0 )
                printf ("%c ", '-');
            else
                printf ("%c ", query_iterator[i-1]);
        
            for (size_t j = 0; j < COLUMNS; ++j)
            {
                printf ("%*d ", print_width, matrix[i*COLUMNS + j]);
            }
            printf ("\n");
        }
    }

    void print_indel (char const* name, char const* text, size_t text_size, int indel_xstart, int indel_end)
    {
        printf ( "%s: %.*s[%.*s]%.*s\n",
                    name,
                    (indel_xstart + 1), text,
                    indel_end - (indel_xstart + 1), text + (indel_xstart + 1),
                    (int)(text_size - indel_end), text + indel_end
               );
    }

    enum ColumnNameIndices
    {
//        idx_SEQ_LEN,
//        idx_SEQ_START,
        idx_MAX_SEQ_LEN,
        idx_TOTAL_SEQ_LEN,
        idx_READ
    };
    char const* g_ColumnNames[] =
    {
//        "SEQ_LEN",
//        "SEQ_START", // 1-based
        "MAX_SEQ_LEN",
        "TOTAL_SEQ_LEN",
        "READ"
    };
    uint32_t g_ColumnIndex [ countof (g_ColumnNames) ];


    std::string get_ref_slice_int (
                            VDBObjects::CVCursor const& cursor,
                            int64_t ref_start,
                            int64_t ref_end,
                            uint32_t max_seq_len
                           )
    {
        int64_t ref_start_id = ref_start / max_seq_len + 1;
        int64_t ref_start_chunk_pos = ref_start % max_seq_len;

        int64_t ref_end_id = ref_end / max_seq_len + 1;
        int64_t ref_end_chunk_pos = ref_end % max_seq_len;

        std::string ret;
        ret.reserve( ref_end - ref_start + 2 );

        for ( int64_t id = ref_start_id; id <= ref_end_id; ++id )
        {
            int64_t chunk_pos_start = id == ref_start_id ?
                        ref_start_chunk_pos : 0;
            int64_t chunk_pos_end = id == ref_end_id ?
                        ref_end_chunk_pos : max_seq_len - 1;

            char const* pREAD;
            uint32_t count =  cursor.CellDataDirect ( id, g_ColumnIndex[idx_READ], & pREAD );
            assert (count > ref_end_chunk_pos);
            (void)count;

            ret.append (pREAD + chunk_pos_start, pREAD + chunk_pos_end + 1);
        }

        return ret;
    }

    std::string get_ref_slice ( VDBObjects::CVCursor const& cursor,
                                int64_t ref_pos_var, // reported variation position on the reference
                                int64_t var_len,     // the length of the reported variation
                                int64_t slice_expand_left,
                                int64_t slice_expand_right,
                                int64_t* ref_slice_start, // returned value
                                int64_t* ref_slice_end    // returned value, inclusive
                              )
    {
        int64_t idRow = 0;
        uint64_t nRowCount = 0;

        cursor.GetIdRange (idRow, nRowCount);

        uint32_t max_seq_len;
        uint64_t total_seq_len;
        cursor.ReadItems ( idRow, g_ColumnIndex[idx_MAX_SEQ_LEN], & max_seq_len, 1 );
        cursor.ReadItems ( idRow, g_ColumnIndex[idx_TOTAL_SEQ_LEN], & total_seq_len, 1 );

        if ( (uint64_t) g_Params.ref_pos_var > total_seq_len )
        {
            throw Utils::CErrorMsg ("reference position (%ld) is greater than total sequence length (%ud)",
                                    g_Params.ref_pos_var, total_seq_len);
        }

        int64_t var_half_len = var_len / 2 + 1;

        int64_t ref_start = ref_pos_var - var_half_len - slice_expand_left >= 0 ?
                            ref_pos_var - var_half_len - slice_expand_left : 0;
        int64_t ref_end = (uint64_t)(ref_pos_var + var_half_len + slice_expand_right - 1) < total_seq_len ?
                          ref_pos_var + var_half_len + slice_expand_right - 1 : total_seq_len - 1;

        std::string ret = get_ref_slice_int ( cursor, ref_start, ref_end, max_seq_len );

        if ( ref_slice_start != NULL )
            *ref_slice_start = ref_start;
        if ( ref_slice_end != NULL )
            *ref_slice_end = ref_end;

        return ret;
    }

    // now it works for pure insertions (no mismatches/deletions)
    std::string make_query ( std::string const& ref_slice,
                             char const* variation, size_t var_len,
                             int64_t var_start_pos_adj // ref_pos adjusted to the beginning of ref_slice (in the simplest case - the middle of ref_slice)
                           )
    {
        std::string ret;
        ret.reserve ( ref_slice.size() + var_len );

        ret.append ( ref_slice.begin(), ref_slice.begin() + var_start_pos_adj );
        ret.append ( variation, variation + var_len );
        ret.append ( ref_slice.begin() + var_start_pos_adj, ref_slice.end() );

        return ret;
    }

    void find_indel_box_coordinates (
                          std::string const& ref_slice,
                          std::string const& query,
                          int& ref_xstart, int& ref_end,
                          int& query_xstart, int& query_end
                          )
    {
        // building sw-matrix for chosen reference slice and sequence

        size_t COLUMNS = ref_slice.size() + 1;
        size_t ROWS = query.size() + 1;

        int& row_start = query_xstart;
        int& col_start = ref_xstart;
        int& row_end = query_end;
        int& col_end = ref_end;

        std::vector<int> matrix( ROWS * COLUMNS );

        // forward scan
        calculate_similarity_matrix<false> ( query.c_str(), query.size(), ref_slice.c_str(), ref_slice.size(), &matrix[0] );
        //print_matrix<reverse> (&matrix[0], ref_slice.c_str(), ref_slice.size(), query.c_str(), query.size());
        sw_find_indel_box ( & matrix[0], ROWS, COLUMNS, &row_start, &row_end, &col_start, &col_end );

        // reverse scan
        calculate_similarity_matrix<true> ( query.c_str(), query.size(), ref_slice.c_str(), ref_slice.size(), &matrix[0] );
        int row_start_rev, col_start_rev, row_end_rev, col_end_rev;
        sw_find_indel_box ( & matrix[0], ROWS, COLUMNS, &row_start_rev, &row_end_rev, &col_start_rev, &col_end_rev );

        row_start = min ( (int)query.size() - row_end_rev - 1, row_start );
        row_end   = max ( (int)query.size() - row_start_rev - 1, row_end );
        col_start = min ( (int)ref_slice.size() - col_end_rev - 1, col_start );
        col_end   = max ( (int)ref_slice.size() - col_start_rev - 1, col_end );
    }

    std::string find_variation_region_impl ()
    {
        VDBObjects::CVDBManager mgr;
        mgr.Make();

        VDBObjects::CVTable table = mgr.OpenTable ( g_Params.ref_acc );
        VDBObjects::CVCursor cursor = table.CreateCursorRead ();

        cursor.InitColumnIndex (g_ColumnNames, g_ColumnIndex, countof(g_ColumnNames));
        cursor.Open();

        int64_t var_len = strlen (g_Params.query);
        int64_t slice_start = -1, slice_end = -1;
        int64_t add_l = 0, add_r = 0;

        while ( true )
        {
            int64_t new_slice_start, new_slice_end;
            std::string ref_slice = get_ref_slice (cursor, g_Params.ref_pos_var, var_len, add_l, add_r, & new_slice_start, & new_slice_end );
            std::string query = make_query ( ref_slice, g_Params.query, var_len, g_Params.ref_pos_var - new_slice_start );

            std::cout
                << "Looking for query \"" << query
                << "\" at the reference around pos=" << g_Params.ref_pos_var
                << " [" << new_slice_start << ", " << new_slice_end << "]"
                << ": \"" << ref_slice << "\"..." << std::endl;

            int ref_xstart, ref_end, query_xstart, query_end;
            find_indel_box_coordinates (ref_slice, query, ref_xstart, ref_end, query_xstart, query_end );

            printf ("indel box found: (%d, %d) - (%d, %d)\n", query_xstart, ref_xstart, query_end, query_end );

            print_indel ( "reference", ref_slice.c_str(), ref_slice.size(), ref_xstart, ref_end );
            print_indel ( "query    ", query.c_str(), query.size(), query_xstart, query_end );

            bool cont = false;

            if ( ref_xstart == -1)
            {
                if ( slice_start != -1 && new_slice_start == slice_start )
                    std::cout << "cannot expand to the left anymore" <<std::endl;
                else
                {
                    std::cout << "expanding the window to the left..." << std::endl;
                    add_l += 2;
                    cont = true;
                }
            }
            if ( ref_end == (int64_t)ref_slice.size() )
            {
                if ( slice_end != -1 && new_slice_end == slice_end )
                    std::cout << "cannot expand to the right anymore" <<std::endl;
                else
                {
                    std::cout << "expanding the window to the right..." << std::endl;
                    add_r += 2;
                    cont = true;
                }
            }

            if ( !cont )
                return query.substr( query_xstart + 1, query_end - query_xstart - 1 );

            slice_start = new_slice_start;
            slice_end = new_slice_end;
        }

        //printf ("Reference slice [%ld, %ld]: %s\n", slice_start, slice_end, ref_slice.c_str() );

    } 

    void find_variation_region (int argc, char** argv)
    {
        try
        {
            KApp::CArgs args (argc, argv, Options, countof (Options));
            uint32_t param_count = args.GetParamCount ();
            if ( param_count != 0 )
            {
                MiniUsage (args.GetArgs());
                return;
            }

            // Actually GetOptionCount check is not needed here since
            // CArgs checks that exactly 1 option is required

            if (args.GetOptionCount (OPTION_REFERENCE_ACC) == 1)
                g_Params.ref_acc = args.GetOptionValue ( OPTION_REFERENCE_ACC, 0 );

            if (args.GetOptionCount (OPTION_REF_POS) == 1)
                g_Params.ref_pos_var = args.GetOptionValueInt<int64_t> ( OPTION_REF_POS, 0 );

            if (args.GetOptionCount (OPTION_QUERY) == 1)
                g_Params.query = args.GetOptionValue ( OPTION_QUERY, 0 );

            g_Params.verbosity = (int)args.GetOptionCount (OPTION_VERBOSITY);

            std::string full_var = find_variation_region_impl ();

            std::cout << "Full variation: " << full_var << std::endl;
        }
        catch (...)
        {
            Utils::HandleException ();
        }
    }
}

extern "C"
{
    const char UsageDefaultName[] = "ref-variation";
    ver_t CC KAppVersion ()
    {
        return REF_VARIATION_VERS;
    }

    rc_t CC UsageSummary (const char * progname)
    {
        printf (
        "Usage example:\n"
        "  %s -r <reference accession> -p <position on reference> -q <query to look for>\n"
        "\n"
        "Summary:\n"
        "  Find a possible indel window\n"
        "\n", progname);
        return 0;
    }

    rc_t CC Usage ( struct Args const * args )
    {
        rc_t rc = 0;
        const char* progname = UsageDefaultName;
        const char* fullpath = UsageDefaultName;

        if (args == NULL)
            rc = RC(rcExe, rcArgv, rcAccessing, rcSelf, rcNull);
        else
            rc = ArgsProgram(args, &fullpath, &progname);

        UsageSummary (progname);

        printf ("\nOptions:\n");

        HelpOptionLine (RefVariation::ALIAS_REFERENCE_ACC, RefVariation::OPTION_REFERENCE_ACC, "acc", RefVariation::USAGE_REFERENCE_ACC);
        HelpOptionLine (RefVariation::ALIAS_REF_POS, RefVariation::OPTION_REF_POS, "value", RefVariation::USAGE_REF_POS);
        HelpOptionLine (RefVariation::ALIAS_QUERY, RefVariation::OPTION_QUERY, "string", RefVariation::USAGE_QUERY);
        //HelpOptionLine (RefVariation::ALIAS_VERBOSITY, RefVariation::OPTION_VERBOSITY, "", RefVariation::USAGE_VERBOSITY);

        printf ("\n");

        HelpOptionsStandard ();

        HelpVersion (fullpath, KAppVersion());

        return rc;
    }

    rc_t CC KMain ( int argc, char *argv [] )
    {
        /* command line examples:
          -r NC_011752.1 -p 2018 -q CA
          -r NC_011752.1 -p 2020 -q CA
          -r NC_011752.1 -p 5000 -q CA*/

        RefVariation::find_variation_region ( argc, argv );
        return 0;
    }
}