/* $Id: downmix.c,v 1.15 2005/03/17 19:22:47 stebbins Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "common.h"
#include "downmix.h"

#define LVL_PLUS6DB 2.0
#define LVL_PLUS3DB 1.4142135623730951
#define LVL_3DB 0.7071067811865476
#define LVL_45DB 0.5946035575013605
#define LVL_6DB 0.5

#define LVL_SQRT_1_3 0.577350269
#define LVL_SQRT_2_3 0.816496581

#define HB_CH_FRONT_LEFT             0x00000001
#define HB_CH_FRONT_RIGHT            0x00000002
#define HB_CH_FRONT_CENTER           0x00000004
#define HB_CH_LOW_FREQUENCY          0x00000008
#define HB_CH_BACK_LEFT              0x00000010
#define HB_CH_BACK_RIGHT             0x00000020
#define HB_CH_BACK_CENTER            0x00000040
#define HB_CH_SIDE_LEFT              0x00000080
#define HB_CH_SIDE_RIGHT             0x00000100

#define HB_CH_SURROUND_MASK          0x000001f0
#define HB_CH_MASK                   0x000007ff

#define HB_CH_DOLBY                  0x00000800
#define HB_CH_DPLII                  0x00001000

#define DOWNMIX_MONO 0
#define DOWNMIX_STEREO 1
#define DOWNMIX_3F 2
#define DOWNMIX_2F1R 3
#define DOWNMIX_3F1R 4
#define DOWNMIX_2F2R 5
#define DOWNMIX_3F2R 6
#define DOWNMIX_3F4R 7
#define DOWNMIX_DOLBY 8
#define DOWNMIX_DPLII 9
#define DOWNMIX_NUM_MODES 10

#define DOWNMIX_CHANNEL_MASK 0x0f

#define DOWNMIX_LFE_FLAG 0x10
#define DOWNMIX_FLAGS_MASK 0x10

hb_sample_t downmix_matrix[DOWNMIX_NUM_MODES][DOWNMIX_NUM_MODES][8][8] =
{
// MONO in
{
    // MONO out
    { { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // STEREO out
    { { LVL_3DB, LVL_3DB, 0, 0, 0, 0, 0, 0 },
      { 0,       0,       1, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 } },
    // 3F out
    { { 0, LVL_3DB, LVL_3DB, 0, 0, 0, 0, 0 },
      { 0, 0,       0,       1, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 } },
    // 2F1R out
    { { LVL_3DB, LVL_3DB, 0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 1, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 } },
    // 3F1R out
    { { 0, LVL_3DB, LVL_3DB, 0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 1, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 } },
    // 2F2R out
    { { LVL_3DB, LVL_3DB, 0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 1, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 } },
    // 3F2R out
    { { 0, LVL_3DB, LVL_3DB, 0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 1, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 } },
    // 3F4R out
    { { 0, LVL_3DB, LVL_3DB, 0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 1 },
      { 0, 0,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 } },
    // DOLBY out
    { { LVL_3DB, LVL_3DB, 0, 0, 0, 0, 0, 0 },
      { 0,       0,       1, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 } },
    // DPLII out
    { { LVL_3DB, LVL_3DB, 0, 0, 0, 0, 0, 0 },
      { 0,       0,       1, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 } },
},
// STEREO in
{
    // MONO out
    { { LVL_3DB, 0, 0, 0, 0, 0, 0, 0 },
      { LVL_3DB, 0, 0, 0, 0, 0, 0, 0 },
      {       0, 1, 0, 0, 0, 0, 0, 0 },
      {       0, 0, 0, 0, 0, 0, 0, 0 },
      {       0, 0, 0, 0, 0, 0, 0, 0 },
      {       0, 0, 0, 0, 0, 0, 0, 0 },
      {       0, 0, 0, 0, 0, 0, 0, 0 },
      {       0, 0, 0, 0, 0, 0, 0, 0 } },
    // STEREO out
    { { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // 3F out
    { { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 1, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // 2F1R out
    { { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 1, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // 3F1R out
    { { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 1, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // 2F2R out
    { { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 1, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // 3F2R out
    { { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 1, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // 3F4R out
    { { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 1 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // DOLBY out
    { { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // DPLII out
    { { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
},
// 3F in
{
    // MONO out
    { { LVL_PLUS3DB, 0, 0, 0, 0, 0, 0, 0 },
      { LVL_3DB,     0, 0, 0, 0, 0, 0, 0 },
      { LVL_3DB,     0, 0, 0, 0, 0, 0, 0 },
      { 0,           1, 0, 0, 0, 0, 0, 0 },
      { 0,           0, 0, 0, 0, 0, 0, 0 },
      { 0,           0, 0, 0, 0, 0, 0, 0 },
      { 0,           0, 0, 0, 0, 0, 0, 0 },
      { 0,           0, 0, 0, 0, 0, 0, 0 } },
    // STEREO out
    { { 1, 1, 0, 0, 0, 0, 0, 0 },
      { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // 3F out
    { { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 1, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // 2F1R out
    { { 1, 1, 0, 0, 0, 0, 0, 0 },
      { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 1, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // 3F1R out
    { { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 1, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // 2F2R out
    { { 1, 1, 0, 0, 0, 0, 0, 0 },
      { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 1, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // 3F2R out
    { { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 1, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // 3F4R out
    { { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 1 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // DOLBY out
    { { 1, 1, 0, 0, 0, 0, 0, 0 },
      { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // DPLII out
    { { 1, 1, 0, 0, 0, 0, 0, 0 },
      { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
},
// 2F1R in
{
    // MONO out
    { { LVL_3DB,     0, 0, 0, 0, 0, 0, 0 },
      { LVL_3DB,     0, 0, 0, 0, 0, 0, 0 },
      { LVL_3DB,     0, 0, 0, 0, 0, 0, 0 },
      { 0,           1, 0, 0, 0, 0, 0, 0 },
      { 0,           0, 0, 0, 0, 0, 0, 0 },
      { 0,           0, 0, 0, 0, 0, 0, 0 },
      { 0,           0, 0, 0, 0, 0, 0, 0 },
      { 0,           0, 0, 0, 0, 0, 0, 0 } },
    // STEREO out
    { { 1,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       1,       0, 0, 0, 0, 0, 0 },
      { LVL_3DB, LVL_3DB, 0, 0, 0, 0, 0, 0 },
      { 0,       0,       1, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 } },
    // 3F out
    { { 0, 1,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       1,       0, 0, 0, 0, 0 },
      { 0, LVL_3DB, LVL_3DB, 0, 0, 0, 0, 0 },
      { 0, 0,       0,       1, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 } },
    // 2F1R out
    { { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 1, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // 3F1R out
    { { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 1, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 1, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // 2F2R out
    { { 1, 0, 0,       0,       0, 0, 0, 0 },
      { 0, 1, 0,       0,       0, 0, 0, 0 },
      { 0, 0, LVL_3DB, LVL_3DB, 0, 0, 0, 0 },
      { 0, 0, 0,       0,       1, 0, 0, 0 },
      { 0, 0, 0,       0,       0, 0, 0, 0 },
      { 0, 0, 0,       0,       0, 0, 0, 0 },
      { 0, 0, 0,       0,       0, 0, 0, 0 },
      { 0, 0, 0,       0,       0, 0, 0, 0 } },
    // 3F2R out
    { { 0, 1, 0, 0,       0,       0, 0, 0 },
      { 0, 0, 1, 0,       0,       0, 0, 0 },
      { 0, 0, 0, LVL_3DB, LVL_3DB, 0, 0, 0 },
      { 0, 0, 0, 0,       0,       1, 0, 0 },
      { 0, 0, 0, 0,       0,       0, 0, 0 },
      { 0, 0, 0, 0,       0,       0, 0, 0 },
      { 0, 0, 0, 0,       0,       0, 0, 0 },
      { 0, 0, 0, 0,       0,       0, 0, 0 } },
    // 3F4R out
    { { 0, 1, 0, 0, 0, 0,       0      , 0 },
      { 0, 0, 1, 0, 0, 0,       0      , 0 },
      { 0, 0, 0, 0, 0, LVL_3DB, LVL_3DB, 0 },
      { 0, 0, 0, 0, 0, 0,       0      , 1 },
      { 0, 0, 0, 0, 0, 0,       0      , 0 },
      { 0, 0, 0, 0, 0, 0,       0      , 0 },
      { 0, 0, 0, 0, 0, 0,       0      , 0 },
      { 0, 0, 0, 0, 0, 0,       0      , 0 } },
    // DOLBY out
    { { 1,        0,       0, 0, 0, 0, 0, 0 },
      { 0,        1,       0, 0, 0, 0, 0, 0 },
      { -LVL_3DB, LVL_3DB, 0, 0, 0, 0, 0, 0 },
      { 0,        0,       1, 0, 0, 0, 0, 0 },
      { 0,        0,       0, 0, 0, 0, 0, 0 },
      { 0,        0,       0, 0, 0, 0, 0, 0 },
      { 0,        0,       0, 0, 0, 0, 0, 0 },
      { 0,        0,       0, 0, 0, 0, 0, 0 } },
    // DPLII out
    { { 1,        0,       0, 0, 0, 0, 0, 0 },
      { 0,        1,       0, 0, 0, 0, 0, 0 },
      { -LVL_3DB, LVL_3DB, 0, 0, 0, 0, 0, 0 },
      { 0,        0,       1, 0, 0, 0, 0, 0 },
      { 0,        0,       0, 0, 0, 0, 0, 0 },
      { 0,        0,       0, 0, 0, 0, 0, 0 },
      { 0,        0,       0, 0, 0, 0, 0, 0 },
      { 0,        0,       0, 0, 0, 0, 0, 0 } },
},
// 3F1R in
{
    // MONO out
    { { LVL_PLUS3DB, 0, 0, 0, 0, 0, 0, 0 },
      { LVL_3DB,     0, 0, 0, 0, 0, 0, 0 },
      { LVL_3DB,     0, 0, 0, 0, 0, 0, 0 },
      { LVL_3DB,     0, 0, 0, 0, 0, 0, 0 },
      { 0,           1, 0, 0, 0, 0, 0, 0 },
      { 0,           0, 0, 0, 0, 0, 0, 0 },
      { 0,           0, 0, 0, 0, 0, 0, 0 },
      { 0,           0, 0, 0, 0, 0, 0, 0 } },
    // STEREO out
    { { 1,       1,       0, 0, 0, 0, 0, 0 },
      { 1,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       1,       0, 0, 0, 0, 0, 0 },
      { LVL_3DB, LVL_3DB, 0, 0, 0, 0, 0, 0 },
      { 0,       0,       1, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       0,       0, 0, 0, 0, 0, 0 } },
    // 3F out
    { { 1, 0,       0,       0, 0, 0, 0, 0 },
      { 0, 1,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       1,       0, 0, 0, 0, 0 },
      { 0, LVL_3DB, LVL_3DB, 0, 0, 0, 0, 0 },
      { 0, 0,       0,       1, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       0,       0, 0, 0, 0, 0 } },
    // 2F1R out
    { { 1, 1, 0, 0, 0, 0, 0, 0 },
      { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 1, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // 3F1R out
    { { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 1, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 1, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // 2F2R out
    { { 1, 1, 0,       0,       0, 0, 0, 0 },
      { 1, 0, 0,       0,       0, 0, 0, 0 },
      { 0, 1, 0,       0,       0, 0, 0, 0 },
      { 0, 0, LVL_3DB, LVL_3DB, 0, 0, 0, 0 },
      { 0, 0, 0,       0,       1, 0, 0, 0 },
      { 0, 0, 0,       0,       0, 0, 0, 0 },
      { 0, 0, 0,       0,       0, 0, 0, 0 },
      { 0, 0, 0,       0,       0, 0, 0, 0 } },
    // 3F2R out
    { { 1, 0, 0, 0,       0,       0, 0, 0 },
      { 0, 1, 0, 0,       0,       0, 0, 0 },
      { 0, 0, 1, 0,       0,       0, 0, 0 },
      { 0, 0, 0, LVL_3DB, LVL_3DB, 0, 0, 0 },
      { 0, 0, 0, 0,       0,       1, 0, 0 },
      { 0, 0, 0, 0,       0,       0, 0, 0 },
      { 0, 0, 0, 0,       0,       0, 0, 0 },
      { 0, 0, 0, 0,       0,       0, 0, 0 } },
    // 3F4R out
    { { 1, 0, 0, 0, 0, 0,       0      , 0 },
      { 0, 1, 0, 0, 0, 0,       0      , 0 },
      { 0, 0, 1, 0, 0, 0,       0      , 0 },
      { 0, 0, 0, 0, 0, LVL_3DB, LVL_3DB, 0 },
      { 0, 0, 0, 0, 0, 0,       0      , 1 },
      { 0, 0, 0, 0, 0, 0,       0      , 0 },
      { 0, 0, 0, 0, 0, 0,       0      , 0 },
      { 0, 0, 0, 0, 0, 0,       0      , 0 } },
    // DOLBY out
    { { LVL_3DB,  LVL_3DB, 0, 0, 0, 0, 0, 0 },
      { 1,        0,       0, 0, 0, 0, 0, 0 },
      { 0,        1,       0, 0, 0, 0, 0, 0 },
      { -LVL_3DB, LVL_3DB, 0, 0, 0, 0, 0, 0 },
      { 0,        0,       1, 0, 0, 0, 0, 0 },
      { 0,        0,       0, 0, 0, 0, 0, 0 },
      { 0,        0,       0, 0, 0, 0, 0, 0 },
      { 0,        0,       0, 0, 0, 0, 0, 0 } },
    // DPLII out
    { { LVL_3DB,  LVL_3DB, 0, 0, 0, 0, 0, 0 },
      { 1,        0,       0, 0, 0, 0, 0, 0 },
      { 0,        1,       0, 0, 0, 0, 0, 0 },
      { -LVL_3DB, LVL_3DB, 0, 0, 0, 0, 0, 0 },
      { 0,        0,       1, 0, 0, 0, 0, 0 },
      { 0,        0,       0, 0, 0, 0, 0, 0 },
      { 0,        0,       0, 0, 0, 0, 0, 0 },
      { 0,        0,       0, 0, 0, 0, 0, 0 } },
},
// 2F2R in
{
    // MONO out
    { { LVL_3DB, 0, 0, 0, 0, 0, 0, 0 },
      { LVL_3DB, 0, 0, 0, 0, 0, 0, 0 },
      { LVL_3DB, 0, 0, 0, 0, 0, 0, 0 },
      { LVL_3DB, 0, 0, 0, 0, 0, 0, 0 },
      { 0,       1, 0, 0, 0, 0, 0, 0 },
      { 0,       0, 0, 0, 0, 0, 0, 0 },
      { 0,       0, 0, 0, 0, 0, 0, 0 },
      { 0,       0, 0, 0, 0, 0, 0, 0 } },
    // STEREO out
    { { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // 3F out
    { { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 1, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // 2F1R out
    { { 1, 0, 0,       0, 0, 0, 0, 0 },
      { 0, 1, 0,       0, 0, 0, 0, 0 },
      { 0, 0, LVL_3DB, 0, 0, 0, 0, 0 },
      { 0, 0, LVL_3DB, 0, 0, 0, 0, 0 },
      { 0, 0, 0,       1, 0, 0, 0, 0 },
      { 0, 0, 0,       0, 0, 0, 0, 0 },
      { 0, 0, 0,       0, 0, 0, 0, 0 },
      { 0, 0, 0,       0, 0, 0, 0, 0 } },
    // 3F1R out
    { { 0, 1, 0, 0,       0, 0, 0, 0 },
      { 0, 0, 1, 0,       0, 0, 0, 0 },
      { 0, 0, 0, LVL_3DB, 0, 0, 0, 0 },
      { 0, 0, 0, LVL_3DB, 0, 0, 0, 0 },
      { 0, 0, 0, 0,       1, 0, 0, 0 },
      { 0, 0, 0, 0,       0, 0, 0, 0 },
      { 0, 0, 0, 0,       0, 0, 0, 0 },
      { 0, 0, 0, 0,       0, 0, 0, 0 } },
    // 2F2R out
    { { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 1, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 1, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // 3F2R out
    { { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 1, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 1, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 1, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // 3F4R out
    { { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 1, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 1, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 1 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // DOLBY out
    { { 1,        0,       0, 0, 0, 0, 0, 0 },
      { 0,        1,       0, 0, 0, 0, 0, 0 },
      { -LVL_6DB, LVL_6DB, 0, 0, 0, 0, 0, 0 },
      { -LVL_6DB, LVL_6DB, 0, 0, 0, 0, 0, 0 },
      { 0,        0,       1, 0, 0, 0, 0, 0 },
      { 0,        0,       0, 0, 0, 0, 0, 0 },
      { 0,        0,       0, 0, 0, 0, 0, 0 },
      { 0,        0,       0, 0, 0, 0, 0, 0 } },
    // DPLII out
    { { 1,             0,             0, 0, 0, 0, 0, 0 },
      { 0,             1,             0, 0, 0, 0, 0, 0 },
      { LVL_SQRT_2_3,  -LVL_SQRT_1_3, 0, 0, 0, 0, 0, 0 },
      { -LVL_SQRT_1_3, LVL_SQRT_2_3,  0, 0, 0, 0, 0, 0 },
      { 0,             0,             1, 0, 0, 0, 0, 0 },
      { 0,             0,             0, 0, 0, 0, 0, 0 },
      { 0,             0,             0, 0, 0, 0, 0, 0 },
      { 0,             0,             0, 0, 0, 0, 0, 0 } },
},
// 3F2R in
{
    // MONO out
    { { LVL_PLUS3DB, 0, 0, 0, 0, 0, 0, 0 },
      { LVL_3DB,     0, 0, 0, 0, 0, 0, 0 },
      { LVL_3DB,     0, 0, 0, 0, 0, 0, 0 },
      { LVL_3DB,     0, 0, 0, 0, 0, 0, 0 },
      { LVL_3DB,     0, 0, 0, 0, 0, 0, 0 },
      { 0,           1, 0, 0, 0, 0, 0, 0 },
      { 0,           0, 0, 0, 0, 0, 0, 0 },
      { 0,           0, 0, 0, 0, 0, 0, 0 } },
    // STEREO out
    { { 1, 1, 0, 0, 0, 0, 0, 0 },
      { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // 3F out
    { { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 1, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // 2F1R out
    { { 1, 1, 0,       0, 0, 0, 0, 0 },
      { 1, 0, 0,       0, 0, 0, 0, 0 },
      { 0, 1, 0,       0, 0, 0, 0, 0 },
      { 0, 0, LVL_3DB, 0, 0, 0, 0, 0 },
      { 0, 0, LVL_3DB, 0, 0, 0, 0, 0 },
      { 0, 0, 0,       1, 0, 0, 0, 0 },
      { 0, 0, 0,       0, 0, 0, 0, 0 },
      { 0, 0, 0,       0, 0, 0, 0, 0 } },
    // 3F1R out
    { { 1, 0, 0, 0,       0, 0, 0, 0 },
      { 0, 1, 0, 0,       0, 0, 0, 0 },
      { 0, 0, 1, 0,       0, 0, 0, 0 },
      { 0, 0, 0, LVL_3DB, 0, 0, 0, 0 },
      { 0, 0, 0, LVL_3DB, 0, 0, 0, 0 },
      { 0, 0, 0, 0,       1, 0, 0, 0 },
      { 0, 0, 0, 0,       0, 0, 0, 0 },
      { 0, 0, 0, 0,       0, 0, 0, 0 } },
    // 2F2R out
    { { 1, 1, 0, 0, 0, 0, 0, 0 },
      { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 1, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 1, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // 3F2R out
    { { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 1, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 1, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 1, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // 3F4R out
    { { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 1, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 1, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 1 },
      { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0 } },
    // DOLBY out
    { { LVL_3DB,  LVL_3DB, 0, 0, 0, 0, 0, 0 },
      { 1,        0,       0, 0, 0, 0, 0, 0 },
      { 0,        1,       0, 0, 0, 0, 0, 0 },
      { -LVL_3DB, LVL_3DB, 0, 0, 0, 0, 0, 0 },
      { -LVL_3DB, LVL_3DB, 0, 0, 0, 0, 0, 0 },
      { 0,        0,       1, 0, 0, 0, 0, 0 },
      { 0,        0,       0, 0, 0, 0, 0, 0 },
      { 0,        0,       0, 0, 0, 0, 0, 0 } },
    // DPLII out
    { { LVL_3DB,       LVL_3DB,       0, 0, 0, 0, 0, 0 },
      { 1,             0,             0, 0, 0, 0, 0, 0 },
      { 0,             1,             0, 0, 0, 0, 0, 0 },
      { LVL_SQRT_2_3,  -LVL_SQRT_1_3, 0, 0, 0, 0, 0, 0 },
      { -LVL_SQRT_1_3, LVL_SQRT_2_3,  0, 0, 0, 0, 0, 0 },
      { 0,             0,             1, 0, 0, 0, 0, 0 },
      { 0,             0,             0, 0, 0, 0, 0, 0 },
      { 0,             0,             0, 0, 0, 0, 0, 0 } },
},
// 3F4R in
{
    // MONO out
    { { LVL_PLUS3DB, 0, 0, 0, 0, 0, 0, 0 },
      { LVL_3DB,     0, 0, 0, 0, 0, 0, 0 },
      { LVL_3DB,     0, 0, 0, 0, 0, 0, 0 },
      { LVL_6DB,     0, 0, 0, 0, 0, 0, 0 },
      { LVL_6DB,     0, 0, 0, 0, 0, 0, 0 },
      { LVL_6DB,     0, 0, 0, 0, 0, 0, 0 },
      { LVL_6DB,     0, 0, 0, 0, 0, 0, 0 },
      { 0,           1, 0, 0, 0, 0, 0, 0 } },
    // STEREO out
    { { 1,       1,       0, 0, 0, 0, 0, 0 },
      { 1,       0,       0, 0, 0, 0, 0, 0 },
      { 0,       1,       0, 0, 0, 0, 0, 0 },
      { LVL_3DB, 0,       0, 0, 0, 0, 0, 0 },
      { 0,       LVL_3DB, 0, 0, 0, 0, 0, 0 },
      { LVL_3DB, 0,       0, 0, 0, 0, 0, 0 },
      { 0,       LVL_3DB, 0, 0, 0, 0, 0, 0 },
      { 0,       0,       1, 0, 0, 0, 0, 0 } },
    // 3F out
    { { 1, 0,       0,       0, 0, 0, 0, 0 },
      { 0, 1,       0,       0, 0, 0, 0, 0 },
      { 0, 0,       1,       0, 0, 0, 0, 0 },
      { 0, LVL_3DB, 0,       0, 0, 0, 0, 0 },
      { 0, 0,       LVL_3DB, 0, 0, 0, 0, 0 },
      { 0, LVL_3DB, 0,       0, 0, 0, 0, 0 },
      { 0, 0,       LVL_3DB, 0, 0, 0, 0, 0 },
      { 0, 0,       0,       1, 0, 0, 0, 0 } },
    // 2F1R out
    { { 1, 1, 0,       0, 0, 0, 0, 0 },
      { 1, 0, 0,       0, 0, 0, 0, 0 },
      { 0, 1, 0,       0, 0, 0, 0, 0 },
      { 0, 0, LVL_6DB, 0, 0, 0, 0, 0 },
      { 0, 0, LVL_6DB, 0, 0, 0, 0, 0 },
      { 0, 0, LVL_6DB, 0, 0, 0, 0, 0 },
      { 0, 0, LVL_6DB, 0, 0, 0, 0, 0 },
      { 0, 0, 0,       1, 0, 0, 0, 0 } },
    // 3F1R out
    { { 1, 0, 0, 0,       0, 0, 0, 0 },
      { 0, 1, 0, 0,       0, 0, 0, 0 },
      { 0, 0, 1, 0,       0, 0, 0, 0 },
      { 0, 0, 0, LVL_6DB, 0, 0, 0, 0 },
      { 0, 0, 0, LVL_6DB, 0, 0, 0, 0 },
      { 0, 0, 0, LVL_6DB, 0, 0, 0, 0 },
      { 0, 0, 0, LVL_6DB, 0, 0, 0, 0 },
      { 0, 0, 0, 0,       1, 0, 0, 0 } },
    // 2F2R out
    { { 1, 1, 0,       0,       0, 0, 0, 0 },
      { 1, 0, 0,       0,       0, 0, 0, 0 },
      { 0, 1, 0,       0,       0, 0, 0, 0 },
      { 0, 0, LVL_3DB, 0,       0, 0, 0, 0 },
      { 0, 0, 0,       LVL_3DB, 0, 0, 0, 0 },
      { 0, 0, LVL_3DB, 0,       0, 0, 0, 0 },
      { 0, 0, 0,       LVL_3DB, 0, 0, 0, 0 },
      { 0, 0, 0,       0,       1, 0, 0, 0 } },
    // 3F2R out
    { { 1, 0, 0, 0,       0,       0, 0, 0 },
      { 0, 1, 0, 0,       0,       0, 0, 0 },
      { 0, 0, 1, 0,       0,       0, 0, 0 },
      { 0, 0, 0, LVL_3DB, 0,       0, 0, 0 },
      { 0, 0, 0, 0,       LVL_3DB, 0, 0, 0 },
      { 0, 0, 0, LVL_3DB, 0,       0, 0, 0 },
      { 0, 0, 0, 0,       LVL_3DB, 0, 0, 0 },
      { 0, 0, 0, 0,       0,       1, 0, 0 } },
    // 3F4R out
    { { 1, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 1, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 1, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 1, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 1, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 1, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 1, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 1 } },
    // DOLBY out
    { { LVL_3DB,  LVL_3DB, 0, 0, 0, 0, 0, 0 },
      { 1,        0,       0, 0, 0, 0, 0, 0 },
      { 0,        1,       0, 0, 0, 0, 0, 0 },
      { -LVL_3DB, LVL_3DB, 0, 0, 0, 0, 0, 0 },
      { -LVL_3DB, LVL_3DB, 0, 0, 0, 0, 0, 0 },
      { -LVL_3DB, LVL_3DB, 0, 0, 0, 0, 0, 0 },
      { -LVL_3DB, LVL_3DB, 0, 0, 0, 0, 0, 0 },
      { 0,        0,       1, 0, 0, 0, 0, 0 } },
    // DPLII out
    { { LVL_3DB,               LVL_3DB,               0, 0, 0, 0, 0, 0 },
      { 1,                     0,                     0, 0, 0, 0, 0, 0 },
      { 0,                     1,                     0, 0, 0, 0, 0, 0 },
      { LVL_SQRT_2_3*LVL_3DB,  -LVL_SQRT_1_3*LVL_3DB, 0, 0, 0, 0, 0, 0 },
      { -LVL_SQRT_1_3*LVL_3DB, LVL_SQRT_2_3*LVL_3DB,  0, 0, 0, 0, 0, 0 },
      { LVL_SQRT_2_3*LVL_3DB,  -LVL_SQRT_1_3*LVL_3DB, 0, 0, 0, 0, 0, 0 },
      { -LVL_SQRT_1_3*LVL_3DB, LVL_SQRT_2_3*LVL_3DB,  0, 0, 0, 0, 0, 0 },
      { 0,                     0,                     1, 0, 0, 0, 0, 0 } }
},
};

static int channel_layout_map[DOWNMIX_NUM_MODES] =
{
    // DOWNMIX_MONO 
    (HB_CH_FRONT_CENTER),
    // DOWNMIX_STEREO 
    (HB_CH_FRONT_LEFT|HB_CH_FRONT_RIGHT),
    // DOWNMIX_3F 
    (HB_CH_FRONT_LEFT|HB_CH_FRONT_RIGHT|HB_CH_FRONT_CENTER),
    // DOWNMIX_2F1R 
    (HB_CH_FRONT_LEFT|HB_CH_FRONT_RIGHT|HB_CH_BACK_CENTER),
    // DOWNMIX_3F1R 
    (HB_CH_FRONT_LEFT|HB_CH_FRONT_RIGHT|HB_CH_FRONT_CENTER|HB_CH_BACK_CENTER),
    // DOWNMIX_2F2R 
    (HB_CH_FRONT_LEFT|HB_CH_FRONT_RIGHT|HB_CH_BACK_LEFT|HB_CH_BACK_RIGHT),
    // DOWNMIX_3F2R 
    (HB_CH_FRONT_LEFT|HB_CH_FRONT_RIGHT|HB_CH_FRONT_CENTER|HB_CH_BACK_LEFT|HB_CH_BACK_RIGHT),
    // DOWNMIX_3F4R 
    (HB_CH_FRONT_LEFT|HB_CH_FRONT_RIGHT|HB_CH_FRONT_CENTER|HB_CH_SIDE_LEFT|
     HB_CH_SIDE_RIGHT|HB_CH_BACK_LEFT|HB_CH_BACK_RIGHT),
    // DOWNMIX_DOLBY 
    (HB_CH_FRONT_LEFT|HB_CH_FRONT_RIGHT),
    // DOWNMIX_DPLII 
    (HB_CH_FRONT_LEFT|HB_CH_FRONT_RIGHT)
};

int hb_layout_to_mode(int layout)
{
    int mode;
    switch (layout & HB_INPUT_CH_LAYOUT_DISCRETE_NO_LFE_MASK)
    {
        case HB_INPUT_CH_LAYOUT_MONO:
            mode = DOWNMIX_MONO;
            break;
        case HB_INPUT_CH_LAYOUT_STEREO:
            mode = DOWNMIX_STEREO;
            break;
        case HB_INPUT_CH_LAYOUT_3F:
            mode = DOWNMIX_3F;
            break;
        case HB_INPUT_CH_LAYOUT_2F1R:
            mode = DOWNMIX_2F1R;
            break;
        case HB_INPUT_CH_LAYOUT_3F1R:
            mode = DOWNMIX_3F1R;
            break;
        case HB_INPUT_CH_LAYOUT_2F2R:
            mode = DOWNMIX_2F2R;
            break;
        case HB_INPUT_CH_LAYOUT_3F2R:
            mode = DOWNMIX_3F2R;
            break;
        case HB_INPUT_CH_LAYOUT_4F2R:
            mode = DOWNMIX_3F2R|DOWNMIX_LFE_FLAG;
            break;
        case HB_INPUT_CH_LAYOUT_3F4R:
            mode = DOWNMIX_3F4R;
            break;
        case HB_INPUT_CH_LAYOUT_DOLBY:
            mode = DOWNMIX_STEREO;
            break;
        default:
            mode = DOWNMIX_STEREO;
            break;
    }
    if (layout & HB_INPUT_CH_LAYOUT_DISCRETE_LFE_MASK)
        mode |= DOWNMIX_LFE_FLAG;
    return mode;
}

int hb_mixdown_to_mode(uint32_t mixdown)
{
    switch (mixdown)
    {
        case HB_AMIXDOWN_MONO:
            return DOWNMIX_MONO;
        case HB_AMIXDOWN_STEREO:
            return DOWNMIX_STEREO;
        case HB_AMIXDOWN_DOLBY:
            return DOWNMIX_DOLBY;
        case HB_AMIXDOWN_DOLBYPLII:
            return DOWNMIX_DPLII;
        case HB_AMIXDOWN_6CH:
            return DOWNMIX_3F2R|DOWNMIX_LFE_FLAG;
        default:
            return DOWNMIX_STEREO;
    }
}

// ffmpeg gives us SMPTE channel layout
// We could use this layout and remap channels in encfaac,
// but VLC may have problems with remapping, so lets
// allow remapping to the default QuickTime order which is:
//
// C   L   R   LS  RS  Rls Rrs LFE
//
// This arrangement also makes it possible to use half as
// many downmix matrices since the matrix with and without
// LFE are the same.
//
// Use hb_layout_remap to accomplish this.  For convenience
// I've provided the necessary maps.
//
// SMPTE channel layout
//
// DUAL-MONO      L   R 
// DUAL-MONO-LFE  L   R   LFE
// MONO           M
// MONO-LFE       M   LFE
// STEREO         L   R 
// STEREO-LFE     L   R   LFE
// 3F             L   R   C
// 3F-LFE         L   R   C    LFE
// 2F1            L   R   S
// 2F1-LFE        L   R   LFE  S
// 3F1            L   R   C    S
// 3F1-LFE        L   R   C    LFE S
// 2F2            L   R   LS   RS
// 2F2-LFE        L   R   LFE  LS   RS
// 3F2            L   R   C    LS   RS
// 3F2-LFE        L   R   C    LFE  LS   RS
// 3F4            L   R   C    Rls  Rrs  LS   RS
// 3F4-LFE        L   R   C    LFE  Rls  Rrs  LS   RS
//

// Map Indicies are mode, lfe, channel respectively
int hb_ac3_chan_map[10][2][8] =
{
//     w/o LFE                       w/ LFE
//     C  L  R LS RS Rls Rrs         L  R  C LS RS Rls Rls LFE
    {{ 0,                       }, { 1, 0,                     }}, // MONO
    {{ 0, 1,                    }, { 1, 2, 0,                  }}, // STEREO
    {{ 1, 0, 2,                 }, { 2, 1, 3, 0,               }}, // 3F
    {{ 0, 1, 2,                 }, { 1, 2, 3, 0,               }}, // 2F1R
    {{ 1, 0, 2, 3,              }, { 2, 1, 3, 4, 0,            }}, // 3F1R
    {{ 0, 1, 2, 3,              }, { 1, 2, 3, 4, 0,            }}, // 2F2R
    {{ 1, 0, 2, 3, 4,           }, { 2, 1, 3, 4, 5,  0,        }}, // 3F2R
    {{ 1, 0, 2, 3, 4,  5,  6,   }, { 2, 1, 3, 4, 5,  6,  7,  0 }}, // 3F4R
    {{ 0, 1,                    }, { 0, 1,                     }}, // DOLBY
    {{ 0, 1,                    }, { 0, 1,                     }}  // DPLII
};

int hb_smpte_chan_map[10][2][8] =
{
//     w/o LFE                       w/ LFE
//     L  R  C LS RS Rls Rrs         L  R  C LS RS Rls Rls LFE
    {{ 0,                       }, { 0, 1,                     }}, // MONO
    {{ 0, 1,                    }, { 0, 1, 2,                  }}, // STEREO
    {{ 2, 0, 1,                 }, { 2, 0, 1, 3,               }}, // 3F
    {{ 0, 1, 2,                 }, { 0, 1, 3, 2,               }}, // 2F1R
    {{ 2, 0, 1, 3,              }, { 2, 0, 1, 4, 3,            }}, // 3F1R
    {{ 0, 1, 2, 3,              }, { 0, 1, 3, 4, 2,            }}, // 2F2R
    {{ 2, 0, 1, 3, 4,           }, { 2, 0, 1, 4, 5,  3,        }}, // 3F2R
    {{ 2, 0, 1, 5, 6,  3,  4,   }, { 2, 0, 1, 6, 7,  4,  5,  3 }}, // 3F4R
    {{ 0, 1,                    }, { 0, 1,                     }}, // DOLBY
    {{ 0, 1,                    }, { 0, 1,                     }}  // DPLII
};
static const uint8_t nchans_tbl[] = {1, 2, 3, 3, 4, 4, 5, 7, 2, 2};

// Takes a set of samples and remaps the channel layout
void hb_layout_remap( int (*layouts)[2][8], hb_sample_t * samples, int layout, int nsamples )
{
    int nchans;
    int ii, jj;
    int lfe;
    int * map;
    int mode;
    hb_sample_t tmp[6];

    mode = hb_layout_to_mode(layout);
    lfe = ((mode & DOWNMIX_LFE_FLAG) != 0);
    mode = mode & DOWNMIX_CHANNEL_MASK;
    nchans = nchans_tbl[mode] + lfe;
    map = layouts[mode][lfe];

    for (ii = 0; ii < nsamples; ii++)
    {
        for (jj = 0; jj < nchans; jj++)
        {
            tmp[jj] = samples[jj];
        }
        for (jj = 0; jj < nchans; jj++)
        {
            samples[jj] = tmp[map[jj]];
        }
        samples += nchans;
    }
}

static void matrix_mul(
    hb_sample_t * dst, 
    hb_sample_t * src, 
    int nchans_out, 
    int nchans_in, 
    int nsamples, 
    hb_sample_t (*matrix)[8],
    hb_sample_t bias)
{
    int nn, ii, jj;
    hb_sample_t val;

    for (nn = 0; nn < nsamples; nn++)
    {
        for (ii = 0; ii < nchans_out; ii++)
        {
            val = 0;
            for (jj = 0; jj < nchans_in; jj++)
            {
                val += src[jj] * matrix[jj][ii];
            }
            dst[ii] = val + bias;
        }
        src += nchans_in;
        dst += nchans_out;
    }
}

static void set_level(
    hb_sample_t (*matrix)[8], 
    hb_sample_t clev, 
    hb_sample_t slev, 
    hb_sample_t level, 
    int mode_in, 
    int mode_out)
{
    int ii, jj;
    int spos;
    int layout_in, layout_out;

    for (ii = 0; ii < 8; ii++)
    {
        for (jj = 0; jj < 8; jj++)
        {
            matrix[ii][jj] *= level;
        }
    }
    if (mode_out >= DOWNMIX_DOLBY)
        return;

    spos = 3;
    layout_in = channel_layout_map[mode_in];
    layout_out = channel_layout_map[mode_out];

    if (!(layout_in & HB_CH_FRONT_CENTER))
    {
        spos--;
    }
    else
    {
        if (!(layout_out & HB_CH_FRONT_CENTER))
        {
            for (jj = 0; jj < 8; jj++)
            {
                matrix[0][jj] *= clev;
            }
        }
    }
    if (layout_in & (HB_CH_BACK_LEFT|HB_CH_BACK_RIGHT|HB_CH_BACK_CENTER|HB_CH_SIDE_LEFT|HB_CH_SIDE_RIGHT))
    {
        if (layout_out & (HB_CH_BACK_LEFT|HB_CH_BACK_RIGHT|HB_CH_BACK_CENTER|HB_CH_SIDE_LEFT|HB_CH_SIDE_RIGHT))
        {
            // Note, slev only gets set if input has surround, and output has none.
            return;
        }
    }
    if (layout_in & (HB_CH_SIDE_LEFT|HB_CH_SIDE_RIGHT))
    {
        for (jj = 0; jj < 8; jj++)
        {
            matrix[spos][jj] *= slev;
            matrix[spos+1][jj] *= slev;
        }
        spos += 2;
    }
    else if (layout_in & (HB_CH_BACK_CENTER))
    {
        for (jj = 0; jj < 8; jj++)
        {
            matrix[spos][jj] *= slev;
        }
    }
    if (layout_in & (HB_CH_BACK_LEFT|HB_CH_BACK_RIGHT))
    {
        for (jj = 0; jj < 8; jj++)
        {
            matrix[spos][jj] *= slev;
            matrix[spos+1][jj] *= slev;
        }
    }
}

#define MIXMODE(x,y) (((x)<<4)|(y))
// The downmix operation can result in new sample values that are
// outside the original range of sample values.  If you wish to
// guarantee that the levels to not exceed the original range,
// call this function after initializing downmix and setting 
// your initial levels.
//
// Note that this can result in generally lower volume levels
// in the resulting downmixed audio.
void hb_downmix_adjust_level( hb_downmix_t * downmix )
{
    int ii, jj;
    int mode_in, mode_out;
    hb_sample_t level = 1.0;
    hb_sample_t clev = downmix->clev;
    hb_sample_t slev = downmix->slev;

    mode_in = downmix->mode_in & DOWNMIX_CHANNEL_MASK;
    mode_out = downmix->mode_out & DOWNMIX_CHANNEL_MASK;

    switch MIXMODE(mode_in, mode_out)
    {
    case MIXMODE(DOWNMIX_STEREO, DOWNMIX_MONO):
    case MIXMODE(DOWNMIX_2F2R, DOWNMIX_2F1R):
    case MIXMODE(DOWNMIX_2F2R, DOWNMIX_3F1R):
    case MIXMODE(DOWNMIX_3F2R, DOWNMIX_3F1R):
    case MIXMODE(DOWNMIX_3F4R, DOWNMIX_3F1R):
    case MIXMODE(DOWNMIX_3F4R, DOWNMIX_3F2R):
    level_3db:
        level /= LVL_PLUS3DB;
        break;

    case MIXMODE(DOWNMIX_3F, DOWNMIX_MONO):
        level /= LVL_PLUS3DB + clev * LVL_PLUS3DB;
        break;

    case MIXMODE(DOWNMIX_3F2R, DOWNMIX_2F1R):
    case MIXMODE(DOWNMIX_3F4R, DOWNMIX_2F1R):
    case MIXMODE(DOWNMIX_3F4R, DOWNMIX_2F2R):
        if (1 + clev < LVL_PLUS3DB)
            goto level_3db;
    case MIXMODE(DOWNMIX_3F, DOWNMIX_STEREO):
    case MIXMODE(DOWNMIX_3F, DOWNMIX_2F1R):
    case MIXMODE(DOWNMIX_3F, DOWNMIX_2F2R):
    case MIXMODE(DOWNMIX_3F, DOWNMIX_DOLBY):
    case MIXMODE(DOWNMIX_3F, DOWNMIX_DPLII):
    case MIXMODE(DOWNMIX_3F1R, DOWNMIX_2F1R):
    case MIXMODE(DOWNMIX_3F1R, DOWNMIX_2F2R):
    case MIXMODE(DOWNMIX_3F2R, DOWNMIX_2F2R):
        level /= 1 + clev;
        break;


    case MIXMODE(DOWNMIX_2F1R, DOWNMIX_MONO):
        level /= LVL_PLUS3DB + LVL_3DB * clev;
        break;

    case MIXMODE(DOWNMIX_2F1R, DOWNMIX_DOLBY):
        level /= 1 + LVL_3DB;
        break;

    case MIXMODE(DOWNMIX_2F1R, DOWNMIX_STEREO):
    case MIXMODE(DOWNMIX_2F1R, DOWNMIX_3F):
    case MIXMODE(DOWNMIX_3F1R, DOWNMIX_3F):
        level /= 1 + LVL_3DB * slev;
        break;

    case MIXMODE(DOWNMIX_3F1R, DOWNMIX_MONO):
        level /= LVL_PLUS3DB + LVL_PLUS3DB * clev + LVL_3DB * slev;
        break;

    case MIXMODE(DOWNMIX_3F1R, DOWNMIX_STEREO):
        level /= 1 + clev + LVL_3DB * slev;
        break;

    case MIXMODE(DOWNMIX_3F1R, DOWNMIX_DOLBY):
    case MIXMODE(DOWNMIX_3F1R, DOWNMIX_DPLII):
    case MIXMODE(DOWNMIX_2F2R, DOWNMIX_DOLBY):
        level /= 1 + LVL_PLUS3DB;
        break;

    case MIXMODE(DOWNMIX_2F2R, DOWNMIX_MONO):
        level /= LVL_PLUS3DB + LVL_PLUS3DB * slev;
        break;

    case MIXMODE(DOWNMIX_2F2R, DOWNMIX_STEREO):
    case MIXMODE(DOWNMIX_2F2R, DOWNMIX_3F):
    case MIXMODE(DOWNMIX_3F2R, DOWNMIX_3F):
        level /= 1 + slev;
        break;

    case MIXMODE(DOWNMIX_2F2R, DOWNMIX_DPLII):
        level /= 1 + LVL_SQRT_1_3 + LVL_SQRT_2_3;
        break;

    case MIXMODE(DOWNMIX_3F2R, DOWNMIX_MONO):
    case MIXMODE(DOWNMIX_3F4R, DOWNMIX_MONO):
        level /= LVL_PLUS3DB + LVL_PLUS3DB * clev * LVL_PLUS3DB * slev;
        break;

    case MIXMODE(DOWNMIX_3F2R, DOWNMIX_STEREO):
        level /= 1 + clev + slev;
        break;

    case MIXMODE(DOWNMIX_3F2R, DOWNMIX_DOLBY):
        level /= 1 + 3 * LVL_3DB;
        break;

    case MIXMODE(DOWNMIX_3F2R, DOWNMIX_DPLII):
        level /= 1 + LVL_3DB + LVL_SQRT_1_3 + LVL_SQRT_2_3;
        break;

    case MIXMODE(DOWNMIX_3F4R, DOWNMIX_STEREO):
        level /= 1 + clev + LVL_PLUS3DB * slev;
        break;

    case MIXMODE(DOWNMIX_3F4R, DOWNMIX_3F):
        level /= 1 + LVL_PLUS3DB * slev;
        break;

    case MIXMODE(DOWNMIX_3F4R, DOWNMIX_DOLBY):
        level /= 1 + 5 * LVL_3DB;
        break;

    case MIXMODE(DOWNMIX_3F4R, DOWNMIX_DPLII):
        level /= 1 + LVL_3DB + 2 * LVL_SQRT_1_3 + 2 * LVL_SQRT_2_3;
    }

    for (ii = 0; ii < 8; ii++)
    {
        for (jj = 0; jj < 8; jj++)
        {
            downmix->matrix[ii][jj] *= level;
        }
    }
}

void hb_downmix_set_bias( hb_downmix_t * downmix, hb_sample_t bias )
{
    downmix->bias = bias;
}

// Changes the downmix mode if it needs changing after initialization
int hb_downmix_set_mode( hb_downmix_t * downmix, int layout, int mixdown )
{
    int ii, jj;
    int lfe_in, lfe_out;
    int mode_in, mode_out;
    hb_sample_t (*matrix)[8];

    if ( downmix == NULL )
        return -1;

    mode_in = hb_layout_to_mode(layout);
    mode_out = hb_mixdown_to_mode(mixdown);
    downmix->mode_in = mode_in;
    downmix->mode_out = mode_out;

    mode_in = downmix->mode_in & ~DOWNMIX_FLAGS_MASK;
    mode_out = downmix->mode_out & ~DOWNMIX_FLAGS_MASK;

    if (mode_in >= DOWNMIX_NUM_MODES || mode_out >= DOWNMIX_NUM_MODES)
        return -1;

    matrix = downmix_matrix[mode_in][mode_out];

    for (ii = 0; ii < 8; ii++)
    {
        for (jj = 0; jj < 8; jj++)
        {
            downmix->matrix[ii][jj] = matrix[ii][jj];
        }
    }

    lfe_in = ((downmix->mode_in & DOWNMIX_LFE_FLAG) != 0);
    lfe_out = ((downmix->mode_out & DOWNMIX_LFE_FLAG) != 0);

    downmix->nchans_in = nchans_tbl[mode_in] + lfe_in;
    downmix->nchans_out = nchans_tbl[mode_out] + lfe_out;
    return 0;
}

// Changes the downmix levels if they need changing after initialization
void hb_downmix_set_level( hb_downmix_t * downmix, hb_sample_t clev, hb_sample_t slev, hb_sample_t level )
{
    int ii, jj;
    int mode_in, mode_out;
    hb_sample_t (*matrix)[8];

    if ( downmix == NULL )
        return;

    mode_in = downmix->mode_in & ~DOWNMIX_FLAGS_MASK;
    mode_out = downmix->mode_out & ~DOWNMIX_FLAGS_MASK;

    if (mode_in >= DOWNMIX_NUM_MODES || mode_out >= DOWNMIX_NUM_MODES)
        return;

    matrix = downmix_matrix[mode_in][mode_out];

    for (ii = 0; ii < 8; ii++)
    {
        for (jj = 0; jj < 8; jj++)
        {
            downmix->matrix[ii][jj] = matrix[ii][jj];
        }
    }
    downmix->clev = clev;
    downmix->slev = slev;
    downmix->level = level;
    set_level(downmix->matrix, clev, slev, level, mode_in, mode_out);
}

hb_downmix_t * hb_downmix_init(int layout, int mixdown)
{
    hb_downmix_t * downmix = calloc(1, sizeof(hb_downmix_t));
    
    if (downmix == NULL)
        return NULL;
    if ( hb_downmix_set_mode( downmix, layout, mixdown ) < 0 )
    {
        free( downmix );
        return NULL;
    }
    // Set some good default values
    downmix->clev = LVL_3DB;
    downmix->slev = LVL_3DB;
    downmix->level = 1.0;
    downmix->bias = 0.0;
    set_level(downmix->matrix, LVL_3DB, LVL_3DB, 1.0, 
              downmix->mode_in, downmix->mode_out);
    return downmix;
}

void hb_downmix_close( hb_downmix_t **downmix )
{
    if (*downmix != NULL)
        free(*downmix);
    *downmix = NULL;
}

void hb_downmix( hb_downmix_t * downmix, hb_sample_t * dst, hb_sample_t * src, int nsamples)
{
    matrix_mul(dst, src, downmix->nchans_out, downmix->nchans_in, 
               nsamples, downmix->matrix, downmix->bias);
}

int hb_need_downmix( int layout, int mixdown )
{
    int mode_in, mode_out;

    mode_in = hb_layout_to_mode(layout);
    mode_out = hb_mixdown_to_mode(mixdown);

    return (mode_in != mode_out);
}
