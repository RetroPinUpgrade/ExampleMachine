// Here, a define should be made for every lamp on the machine.
// For demonstration, here are some example lamps that are typical
// for different architectures. This section should be deleted
// when implementing a particular machine.

// Delete all these lamp definitions when implementing a particular
// machine. There's a conditional based on architecture here --
// that should be deleted too. It's just here to provide base
// functionality.
#if (RPU_MPU_ARCHITECTURE<10)
// These example lamp definitions come from Supersonic
#define LAMP_HEAD_SAME_PLAYER_SHOOTS_AGAIN  40
#define LAMP_HEAD_MATCH                     41
#define LAMP_SHOOT_AGAIN                    42
#define LAMP_APRON_CREDIT                   43
#define LAMP_HEAD_BIP                       48
#define LAMP_HEAD_HIGH_SCORE                49
#define LAMP_HEAD_GAME_OVER                 50
#define LAMP_HEAD_TILT                      51
#define LAMP_HEAD_1_PLAYER                  52
#define LAMP_HEAD_2_PLAYER                  53
#define LAMP_HEAD_3_PLAYER                  54
#define LAMP_HEAD_4_PLAYER                  55
#define LAMP_HEAD_PLAYER_1_UP               56
#define LAMP_HEAD_PLAYER_2_UP               57
#define LAMP_HEAD_PLAYER_3_UP               58
#define LAMP_HEAD_PLAYER_4_UP               59
#else
// These example lamp definitions come from Stellar Wars
#define LAMP_SHOOT_AGAIN                    0
#define LAMP_HEAD_1_PLAYER                  49
#define LAMP_HEAD_2_PLAYERS                 50
#define LAMP_HEAD_3_PLAYERS                 51
#define LAMP_HEAD_4_PLAYERS                 52
#define LAMP_HEAD_MATCH                     53
#define LAMP_HEAD_BALL_IN_PLAY              54
#define LAMP_APRON_CREDITS                  55
#define LAMP_HEAD_PLAYER_1_UP               56
#define LAMP_HEAD_PLAYER_2_UP               57
#define LAMP_HEAD_PLAYER_3_UP               58
#define LAMP_HEAD_PLAYER_4_UP               59
#define LAMP_HEAD_TILT                      60
#define LAMP_HEAD_GAME_OVER                 61
#define LAMP_HEAD_SAME_PLAYER_SHOOTS_AGAIN  62
#define LAMP_HEAD_HIGH_SCORE                63
#endif




// Delete all these switch definitions when implementing a particular
// machine. There's a conditional based on architecture here --
// that should be deleted too. It's just here to provide base
// functionality.
#if (RPU_MPU_ARCHITECTURE<10)
#define SW_DROP_3                   2
#define SW_DROP_2                   3
#define SW_DROP_1                   4
#define SW_CREDIT_RESET             5
// Different definitions for Tilt so I can have the same
// code for different architectures
#define SW_TILT                     6
#define SW_PLUMB_TILT               6
#define SW_ROLL_TILT                6
#define SW_OUTHOLE                  7
#define SW_COIN_3                   8
#define SW_COIN_1                   9
#define SW_COIN_2                   10
#define SW_SLAM                     15
#define SW_PLAYFIELD_TILT           15
#define SW_SPINNER                  16
#define SW_SAUCER                   23
#define SW_RIGHT_SLING              30
#define SW_LEFT_SLING               31
#define SW_POP_BUMPER               32
#else
#define SW_PLUMB_TILT             0
#define SW_ROLL_TILT              1
#define SW_CREDIT_RESET           2
#define SW_COIN_1                 3
#define SW_COIN_2                 4
#define SW_COIN_3                 5
#define SW_SLAM                   6
#define SW_HIGH_SCORE_RESET       7
#define SW_SPINNER                12
#define SW_OUTHOLE                19
#define SW_LEFT_SLING             20
#define SW_RIGHT_SLING            21
#define SW_SAUCER                 22
#define SW_DROP_1                 24
#define SW_DROP_2                 25
#define SW_DROP_3                 26
#define SW_POP_BUMPER             35
#define SW_PLAYFIELD_TILT         45
#endif


#define SOL_OUTHOLE                 0
#define SOL_DROP_TARGET_RESET       1
#define SOL_SAUCER                  7
#define SOL_KNOCKER                 13
#define SOLCONT_COIN_LOCKOUT        15

#define SOL_LEFT_SLING              16
#define SOL_RIGHT_SLING             17
#define SOL_POP_BUMPER              18
