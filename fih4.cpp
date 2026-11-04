#include <algorithm>
#include <unordered_map>
#include <iostream>
#include <vector>
#include <cstdint>
#include <cstdlib>
#include <sstream>
#include <cstring>
#include <cctype>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <random>
#include <ctime>
#include <iomanip>
#include <cassert>

//Magics Lookup optimizations
#if defined(__BMI2__) && !defined(NO_PEXT)
    #include <immintrin.h>
    #define USE_PEXT
#endif

#define CHECK(c) do { if (!(c)) { std::cout << "FAIL line " << __LINE__ << ": " #c "\n"; std::exit(1); } } while (0)

using U64 = uint64_t;
using U32 = uint32_t;
using U16 = uint16_t;
using U8  = uint8_t;

using Bitboard = U64;

// ================ Enum's ================ //

enum Color {
    WHITE,
    BLACK
};

enum Square {
    A1 = 0, B1 = 1, C1 = 2, D1 = 3, E1 = 4, F1 = 5, G1 = 6, H1 = 7,
    A2 = 8, B2 = 9, C2 = 10, D2 = 11, E2 = 12, F2 = 13, G2 = 14, H2 = 15,
    A3 = 16, B3 = 17, C3 = 18, D3 = 19, E3 = 20, F3 = 21, G3 = 22, H3 = 23,
    A4 = 24, B4 = 25, C4 = 26, D4 = 27, E4 = 28, F4 = 29, G4 = 30, H4 = 31,
    A5 = 32, B5 = 33, C5 = 34, D5 = 35, E5 = 36, F5 = 37, G5 = 38, H5 = 39,
    A6 = 40, B6 = 41, C6 = 42, D6 = 43, E6 = 44, F6 = 45, G6 = 46, H6 = 47,
    A7 = 48, B7 = 49, C7 = 50, D7 = 51, E7 = 52, F7 = 53, G7 = 54, H7 = 55,
    A8 = 56, B8 = 57, C8 = 58, D8 = 59, E8 = 60, F8 = 61, G8 = 62, H8 = 63,
    NO_SQUARE = 64
};

enum Piece {
    W_PAWN = 0, W_KNIGHT = 1, W_BISHOP = 2, W_ROOK = 3, W_QUEEN = 4,  W_KING = 5,
    B_PAWN = 6, B_KNIGHT = 7, B_BISHOP = 8, B_ROOK = 9, B_QUEEN = 10, B_KING = 11,
    NO_PIECE = 12
};

enum PieceType {
    PAWN = 0, KNIGHT = 1, BISHOP = 2, ROOK = 3, QUEEN = 4, KING = 5, NO_PIECE_TYPE = 6
};

enum MoveType : uint16_t { NORMAL = 0, PROMOTION = 1 << 12, EN_PASSANT = 2 << 12, CASTLING = 3 << 12 };

// ================ Constants ================ //

const int MAX_PLY = 256;
const int MATE = 32000;
const int MATE_BOUND = MATE - MAX_PLY;
const int INF = MATE + 1;

const U64 rookHash[64] = {
    0x8a80104000800020ULL, 0x140002000100040ULL,  0x2801880a0017001ULL,  0x100081001000420ULL,
    0x200020010080420ULL,  0x3001c0002010008ULL,  0x8480008002000100ULL, 0x2080088004402900ULL,
    0x800098204000ULL,     0x2024401000200040ULL, 0x100802000801000ULL,  0x120800800801000ULL,
    0x208808088000400ULL,  0x2802200800400ULL,    0x2200800100020080ULL, 0x801000060821100ULL,
    0x80044006422000ULL,   0x100808020004000ULL,  0x12108a0010204200ULL, 0x140848010000802ULL,
    0x481828014002800ULL,  0x8094004002004100ULL, 0x4010040010010802ULL, 0x20008806104ULL,
    0x100400080208000ULL,  0x2040002120081000ULL, 0x21200680100081ULL,   0x20100080080080ULL,
    0x2000a00200410ULL,    0x20080800400ULL,      0x80088400100102ULL,   0x80004600042881ULL,
    0x4040008040800020ULL, 0x440003000200801ULL,  0x4200011004500ULL,    0x188020010100100ULL,
    0x14800401802800ULL,   0x2080040080800200ULL, 0x124080204001001ULL,  0x200046502000484ULL,
    0x480400080088020ULL,  0x1000422010034000ULL, 0x30200100110040ULL,   0x100021010009ULL,
    0x2002080100110004ULL, 0x202008004008002ULL,  0x20020004010100ULL,   0x2048440040820001ULL,
    0x101002200408200ULL,  0x40802000401080ULL,   0x4008142004410100ULL, 0x2060820c0120200ULL,
    0x1001004080100ULL,    0x20c020080040080ULL,  0x2935610830022400ULL, 0x44440041009200ULL,
    0x280001040802101ULL,  0x2100190040002085ULL, 0x80c0084100102001ULL, 0x4024081001000421ULL,
    0x20030a0244872ULL,    0x12001008414402ULL,   0x2006104900a0804ULL,  0x1004081002402ULL
};
const U64 bishopHash[64] = {
    0x40040844404084ULL,   0x2004208a004208ULL,   0x10190041080202ULL,   0x108060845042010ULL,
    0x581104180800210ULL,  0x2112080446200010ULL, 0x1080820820060210ULL, 0x3c0808410220200ULL,
    0x4050404440404ULL,    0x21001420088ULL,       0x24d0080801082102ULL, 0x1020a0a020400ULL,
    0x40308200402ULL,      0x4011002100800ULL,     0x401484104104005ULL,  0x801010402020200ULL,
    0x400210c3880100ULL,   0x404022024108200ULL,   0x810018200204102ULL,  0x4002801a02003ULL,
    0x85040820080400ULL,   0x810102c808880400ULL,  0xe900410884800ULL,    0x8002020480840102ULL,
    0x220200865090201ULL,  0x2010100a02021202ULL,  0x152048408022401ULL,  0x20080002081110ULL,
    0x4001001021004000ULL, 0x800040400a011002ULL,  0xe4004081011002ULL,   0x1c004001012080ULL,
    0x8004200962a00220ULL, 0x8422100208500202ULL,  0x2000402200300c08ULL, 0x8646020080080080ULL,
    0x80020a0200100808ULL, 0x2010004880111000ULL,  0x623000a080011400ULL, 0x42008c0340209202ULL,
    0x209188240001000ULL,  0x400408a884001800ULL,  0x110400a6080400ULL,   0x1840060a44020800ULL,
    0x90080104000041ULL,   0x201011000808101ULL,   0x1a2208080504f080ULL, 0x8012020600211212ULL,
    0x500861011240000ULL,  0x180806108200800ULL,   0x4000020e01040044ULL, 0x300000261044000aULL,
    0x802241102020002ULL,  0x20906061210001ULL,    0x5a84841004010310ULL, 0x4010801011c04ULL,
    0xa010109502200ULL,    0x4a02012000ULL,         0x500201010098b028ULL, 0x8040002811040900ULL,
    0x28000010020204ULL,   0x6000020202d0240ULL,   0x8918844842082200ULL, 0x4010011029020020ULL
};
const int rookShifts[64] = {
    52, 53, 53, 53, 53, 53, 53, 52,
    53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53,
    52, 53, 53, 53, 53, 53, 53, 52
};
const int bishopShifts[64] = {
    58, 59, 59, 59, 59, 59, 59, 58,
    59, 59, 59, 59, 59, 59, 59, 59,
    59, 59, 57, 57, 57, 57, 59, 59,
    59, 59, 57, 55, 55, 57, 59, 59,
    59, 59, 57, 55, 55, 57, 59, 59,
    59, 59, 57, 57, 57, 57, 59, 59,
    59, 59, 59, 59, 59, 59, 59, 59,
    58, 59, 59, 59, 59, 59, 59, 58
};

// ================ Attack Bitboards ================ //

Bitboard PAWN_ATTACKS[2][64], KNIGHT_ATTACKS[64], KING_ATTACKS[64];
Bitboard ROOK_TABLE[102400], BISHOP_TABLE[5248];
Bitboard BETWEEN[64][64];
Bitboard LINE[64][64];

int rookOffsets[64] = {};
int bishopOffsets[64] = {};

// ================ Constexpr ================ //

constexpr int fileOf(int sq) { return sq & 7; }
constexpr int rankOf(int sq) { return sq >> 3; }
constexpr Bitboard bit(int sq) { return 1ULL << sq; }
constexpr Bitboard  FILE_A = 0x0101010101010101ULL, FILE_B = FILE_A << 1, FILE_G = FILE_A << 6, FILE_H = FILE_A << 7;
constexpr Bitboard north(Bitboard b) { return b << 8; }
constexpr Bitboard south(Bitboard b) { return b >> 8; }
constexpr Bitboard east(Bitboard b)  { return (b << 1) & ~FILE_A; }
constexpr Bitboard west(Bitboard b)  { return (b >> 1) & ~FILE_H; }
constexpr Bitboard northEast(Bitboard b) { return (b << 9) & ~FILE_A; }
constexpr Bitboard northWest(Bitboard b) { return (b << 7) & ~FILE_H; }
constexpr Bitboard southEast(Bitboard b) { return (b >> 7) & ~FILE_A; }
constexpr Bitboard southWest(Bitboard b) { return (b >> 9) & ~FILE_H; }

constexpr Bitboard knightFrom(Bitboard b) {
    Bitboard l1 = (b >> 1) & ~FILE_H,  l2 = (b >> 2) & ~(FILE_G | FILE_H);
    Bitboard r1 = (b << 1) & ~FILE_A,  r2 = (b << 2) & ~(FILE_A | FILE_B);
    Bitboard h1 = l1 | r1, h2 = l2 | r2;
    return (h1 << 16) | (h1 >> 16) | (h2 << 8) | (h2 >> 8);
}
constexpr Bitboard kingFrom(Bitboard b) {
    Bitboard row = b | east(b) | west(b);
    return (row | north(row) | south(row)) & ~b;
}
constexpr Bitboard pawnFrom(Bitboard b, Color c) {
    return c == WHITE ? (northWest(b) | northEast(b)) : (southWest(b) | southEast(b));
}

// ================ Structs ================ //

struct Zobrist { U64 piece[12][64], side, castle[16], ep[8]; } Z;
struct Magic { Bitboard mask; U64 magicHash; int shift; int offset; };

struct Move {
    U16 data = 0;
    Move() = default;
    Move(int f, int t, MoveType mt = NORMAL, int promoCode = 0) :
        data(static_cast<U16>(f | (t << 6) | mt | ((mt == PROMOTION ? promoCode : 0) << 14))) {}
    int from() const { return (data & 0x3F); }
    int to() const { return (data & 0xFC0) >> 6; }
    MoveType mvType() const { return static_cast<MoveType>(data & 0x3000); }
    int promotion() const { return (data >> 14); }
    bool isNull() const { return data == 0; }
    bool operator==(Move other) const { return data == other.data; }
    int  index12() const { return data & 0xFFF; }
};

struct MoveList {
    Move list[256];
    int count = 0;
    void add(Move m) {
        assert(count < 256);
        list[count] = m;
        count++;
    }
};

struct Position {
    Position() { std::fill(Mailbox, Mailbox + 64, U8(NO_PIECE)); }
    Bitboard byType[6] = {};
    Bitboard byColor[2] = {};
    U8 Mailbox[64];
    Color stm = WHITE; //side to move
    U8 ep = NO_SQUARE;
    U8 castling = 0;
    U8 halfmove = 0;
    U16 plies = 0;
    U64 key = 0ULL;
    U64 pawnKey = 0ULL;
    U64 nonPawnKey[2] = {};
    Bitboard checkers = 0ULL;
    Bitboard pinned  = 0ULL;
    Bitboard threats = 0ULL;
};

struct Board {
    Position states[2048];
    int n = 0;

    Position& cur() { return states[n]; }
    const Position& cur() const { return states[n]; }

    void makeMove(Move m);
    void unmakeMove();
    void makeNull();
    void unmakeNull();
    bool hasRepeated(int ply) const;
    bool isDraw(int ply) const;
    U64 keyAfter(Move m) const;
};

Magic rookMagics[64];
Magic bishopMagics[64];

// ================ Inline ================ //

inline int lsb(Bitboard b)                { assert(b); return __builtin_ctzll(b); }
inline int popLsb(Bitboard& b)            { int s = lsb(b); b &= b - 1; return s; }
inline int popcount(Bitboard b)           { return __builtin_popcountll(b); }
inline Piece makePiece(Color c, int type) { return static_cast<Piece>(type == NO_PIECE_TYPE ? NO_PIECE : (c * 6 + type)); }
inline PieceType typeOf(Piece p)          { assert(p != NO_PIECE); return PieceType(p % 6); }
inline Color colorOf(Piece p)             { assert(p != 12); return static_cast<Color>(p / 6); }
inline Color flipColor(Color c)           { return static_cast<Color>(c ^ 1); }

inline int magicIndex(const Magic& m, Bitboard occ) {
    #ifdef USE_PEXT
        return m.offset + int(_pext_u64(occ, m.mask));
    #else
        return m.offset + int(((occ & m.mask) * m.magicHash) >> m.shift);
    #endif
}

inline Bitboard rookAttacks(int sq, Bitboard occ)   { return ROOK_TABLE[magicIndex(rookMagics[sq], occ)]; }
inline Bitboard bishopAttacks(int sq, Bitboard occ) { return BISHOP_TABLE[magicIndex(bishopMagics[sq], occ)]; }
inline Bitboard queenAttacks(int sq, Bitboard occ)  { return rookAttacks(sq, occ) | bishopAttacks(sq, occ); }

// ================ Global Functions ================ //

int strSqtoNum(std::string sq) {
    char file = sq[0];
    char rank = sq[1];

    int col = file - 'a';
    int row = rank - '1';

    return col + (row * 8);
}

void printBbHex(Bitboard bb) {
    std::cout << "0x" 
              << std::hex 
              << std::setw(16) 
              << std::setfill('0') 
              << bb 
              << std::endl;
}

std::string toUCI(Move m) {
    const std::string strRanks[8] = {"a", "b", "c", "d", "e", "f", "g", "h"};
    const std::string strFiles[8] = {"1", "2", "3", "4", "5", "6", "7", "8"};

    std::string strMove = "";
    strMove = strRanks[m.from() % 8] + strFiles[m.from() / 8];
    strMove += strRanks[m.to() % 8] + strFiles[m.to() / 8];

    return m.isNull() ? "0000" : strMove;
}

Move fromUCI(const Position& p, const std::string& s) {
    int fromSQ = strSqtoNum(s.substr(0, 1));
    int toSQ   = strSqtoNum(s.substr(2, 3));
    MoveType mvType = NORMAL;
    int promo = 0;

    if (s.size() != 4 && s.size() != 5) return Move();

    if (std::isalpha(static_cast<unsigned char>(s.back()))) {
        mvType = PROMOTION;
        if (s.back() == 'q') {
            promo = 3;
        }
        else if (s.back() == 'r') {
            promo = 2;
        }
        else if (s.back() == 'b') {
            promo = 1;
        }
    }

    else if (((p.byColor[p.stm] & p.byType[KING]) & bit(fromSQ)) &&
             (bit(toSQ) & 0x4400000000000044)) {
        mvType = CASTLING;
    }

    else if ((bit(fromSQ) & (p.byColor[p.stm] & p.byType[PAWN])) && 
             (bit(toSQ) & ((p.byColor[flipColor(p.stm)] & p.byType[PAWN]) == 0ULL))) {
        mvType = EN_PASSANT;
    }

    Move m(fromSQ, toSQ, mvType, promo);

    return m;
}

// Needs work
void putPiece(Position& p, Piece pc, int sq) {
    p.byColor[p.stm] ^= bit(sq);
    p.byType[pc] ^= bit(sq);
    p.Mailbox[sq] = static_cast<int>(pc);

}

void removePiece(Position& p, int sq) {

}

void movePiece(Position& p, int from, int to) {
    removePiece(p, from);
    putPiece(p, static_cast<Piece>(p.Mailbox[from]), to);
}
// End of needs work

bool isCapture(const Position& p, Move m) {
    return (bit(m.to()) & p.byColor[flipColor(p.stm)]) != 0 || m.mvType() == EN_PASSANT;
}
bool isNoisy(const Position& p, Move m) {
    return (isCapture(p, m) || m.mvType() == PROMOTION);
}
bool isUnderPromotion(Move m) {
    return (m.mvType() == PROMOTION && (m.promotion() != 3));
}

Bitboard slowSlide(int sq, Bitboard occ, bool rook) {
    static const int RD[4][2] = {{1,0},{-1,0},{0,1},{0,-1}}, BD[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
    const int (&D)[4][2] = rook ? RD : BD;
    Bitboard res = 0;
    for (int d = 0; d < 4; d++) {
        int r = rankOf(sq) + D[d][0], f = fileOf(sq) + D[d][1];
        while (r >= 0 && r < 8 && f >= 0 && f < 8) {
            res |= bit(r * 8 + f);
            if (occ & bit(r * 8 + f)) break;
            r += D[d][0]; f += D[d][1];
        }
    }
    return res;
}
void fillTable(const Magic* magics, Bitboard* table, bool rook) {
    for (int sq = 0; sq < 64; sq++) {
        const Magic& m = magics[sq];
        Bitboard sub = 0;
        do {
            table[magicIndex(m, sub)] = slowSlide(sq, sub, rook);
            sub = (sub - m.mask) & m.mask;
        } while (sub);
    }
}

void initTables() {
    static bool done = false;
    if (done) return;
    done = true;
    for (int sq = 0; sq < 64; sq++) { // Pawn attack tables gen
        PAWN_ATTACKS[WHITE][sq] = pawnFrom(bit(sq), WHITE);
        PAWN_ATTACKS[BLACK][sq] = pawnFrom(bit(sq), BLACK);
    }
    for (int sq = 0; sq < 64; sq++) { // Knight attack tables gen
        KNIGHT_ATTACKS[sq] = knightFrom(bit(sq));
    }
    for (int sq = 0; sq < 64; sq++) { // King attack tables gen
        KING_ATTACKS[sq] = kingFrom(bit(sq));
    }
    for (int a = 0; a < 64; a++) {
        for (int b = 0; b < 64; b++) {
            BETWEEN[a][b] = 0ULL;
            LINE[a][b] = 0ULL;
            if (a == b) continue;

            if (slowSlide(a, 0, true) & bit(b)) {
                BETWEEN[a][b] = slowSlide(a, bit(b), true) & slowSlide(b, bit(a), true);
                LINE[a][b] = (slowSlide(a, 0, true) & slowSlide(b, 0, true)) | bit(a) | bit(b);
            }
            else if (slowSlide(a, 0, false) & bit(b)) {
                BETWEEN[a][b] = slowSlide(a, bit(b), false) & slowSlide(b, bit(a), false);
                LINE[a][b] = (slowSlide(a, 0, false) & slowSlide(b, 0, false)) | bit(a) | bit(b);
            }
        }
    }

    // Magic init
    rookOffsets[0] = 0;
    bishopOffsets[0] = 0;
    for (int i = 1; i < 64; i++) { rookOffsets[i] = rookOffsets[i - 1] + (1 << (64 - rookShifts[i - 1])); }
    for (int i = 1; i < 64; i++) { bishopOffsets[i] = bishopOffsets[i - 1] + (1 << (64 - bishopShifts[i - 1])); }

    for (int sq = 0; sq < 64; sq++) {
        constexpr Bitboard RANK_1 = 0x00000000000000FFULL;
        constexpr Bitboard RANK_8 = 0xFF00000000000000ULL;

        const Bitboard rankMask = 0xFFULL << (rankOf(sq) * 8);
        const Bitboard fileMask = FILE_A << fileOf(sq);

        const Bitboard edges =
            ((RANK_1 | RANK_8) & ~rankMask) |
            ((FILE_A | FILE_H) & ~fileMask);

        rookMagics[sq].mask = slowSlide(sq, 0, true) & ~edges;
        rookMagics[sq].magicHash = rookHash[sq];
        rookMagics[sq].shift = rookShifts[sq];
        rookMagics[sq].offset = rookOffsets[sq];

        bishopMagics[sq].mask = slowSlide(sq, 0, false) & ~edges;
        bishopMagics[sq].magicHash = bishopHash[sq];
        bishopMagics[sq].shift = bishopShifts[sq];
        bishopMagics[sq].offset = bishopOffsets[sq];
    }
    fillTable(rookMagics, ROOK_TABLE, true);
    fillTable(bishopMagics, BISHOP_TABLE, false);

    std::mt19937_64 rng(2147483647ULL);
    for (int p = 0; p < 12; p++) {
        for (int sq = 0; sq < 64; sq++) Z.piece[p][sq] = rng();
    }
    Z.side = rng();
    for (int i = 0; i < 16; i++) Z.castle[i] = rng();
    for (int i = 0; i < 8;  i++) Z.ep[i]     = rng();
}

// ================ Debug Functions ================ //

void testBits(Bitboard b) {
    for (int r = 0; r < 8; r++) {
        for (int f = 0; f < 8; f++) {
            U64 val = (b >> ((56 - r*8) + f)) & 1ULL;
            std::cout << val << " ";
        }
        std::cout << "\n";
    }
}

void l1Test() {
    // this was written by AI since Im not smart enough to do all that yet.
    // 1. Leapers and pawns vs plain coordinate loops
    for (int sq = 0; sq < 64; sq++) {
        int r = rankOf(sq), f = fileOf(sq);
        Bitboard kn = 0, ki = 0, pw = 0, pb = 0;
        for (int dr = -2; dr <= 2; dr++)
            for (int df = -2; df <= 2; df++) {
                int rr = r + dr, ff = f + df;
                if (rr < 0 || rr > 7 || ff < 0 || ff > 7) continue;
                int adr = abs(dr), adf = abs(df);
                if (adr * adf == 2) kn |= bit(rr * 8 + ff);                       // L-shape
                if (adr <= 1 && adf <= 1 && (adr || adf)) ki |= bit(rr * 8 + ff); // 3x3 minus self
            }
        if (r < 7) { if (f > 0) pw |= bit(sq + 7); if (f < 7) pw |= bit(sq + 9); }
        if (r > 0) { if (f > 0) pb |= bit(sq - 9); if (f < 7) pb |= bit(sq - 7); }
        CHECK(KNIGHT_ATTACKS[sq] == kn);
        CHECK(KING_ATTACKS[sq] == ki);
        CHECK(PAWN_ATTACKS[WHITE][sq] == pw);
        CHECK(PAWN_ATTACKS[BLACK][sq] == pb);
    }

    // 2. Magic lookups vs slowSlide on 4 million random (square, occupancy) pairs
    std::mt19937_64 rng(12345);                                  // fixed seed = repeatable
    for (int i = 0; i < 4000000; i++) {
        int sq = int(rng() % 64);
        Bitboard occ = (i % 3 == 0) ? rng() : (i % 3 == 1) ? (rng() & rng()) : (rng() & rng() & rng());
        CHECK(rookAttacks(sq, occ)   == slowSlide(sq, occ, true));
        CHECK(bishopAttacks(sq, occ) == slowSlide(sq, occ, false));
    }

    // 3. BETWEEN / LINE sanity
    for (int a = 0; a < 64; a++)
        for (int b = 0; b < 64; b++) {
            CHECK(BETWEEN[a][b] == BETWEEN[b][a]);
            CHECK(LINE[a][b] == LINE[b][a]);
            CHECK(!(BETWEEN[a][b] & (bit(a) | bit(b))));         // never includes the endpoints
            if (LINE[a][b]) CHECK((LINE[a][b] & bit(a)) && (LINE[a][b] & bit(b)));
        }

    // 4. Zobrist keys: all non-zero and all distinct
    std::vector<U64> keys;
    for (int p = 0; p < 12; p++)
        for (int sq = 0; sq < 64; sq++) keys.push_back(Z.piece[p][sq]);
    keys.push_back(Z.side);
    for (int i = 0; i < 16; i++) keys.push_back(Z.castle[i]);
    for (int i = 0; i < 8;  i++) keys.push_back(Z.ep[i]);
    std::sort(keys.begin(), keys.end());
    CHECK(keys.front() != 0);
    for (size_t i = 1; i < keys.size(); i++) CHECK(keys[i] != keys[i - 1]);
    std::cout << "L1 Test [OK]\n";
}

void l2Test() {
    const MoveType types[4] = { NORMAL, PROMOTION, EN_PASSANT, CASTLING };
    for (int from = 0; from < 64; from++) {
        for (int to = 0; to < 64; to++) {
            for (int mvt = 0; mvt < 4; mvt++) {
                for (int promoCode = 0; promoCode < 4; promoCode++) {
                    Move m(from, to, types[mvt], promoCode);
                    CHECK(m.from() == from);
                    CHECK(m.to() == to);
                    CHECK(m.mvType() == types[mvt]);
                    CHECK(m.promotion() == (types[mvt] == PROMOTION ? promoCode : 0));
                }
            }
        }
    }

    Move nullTest;
    CHECK(nullTest.isNull() == true);

    std::cout << "L2 Test [OK]\n";
}

void verifyTests() { l1Test(); l2Test(); }

// ================ Main ================ //

int main(int argc, char** argv) {
    initTables();
    std::string cmd = (argc > 1) ? argv[1] : "uci";
    if (cmd == "test") { verifyTests(); }
    Move m(0, 8);

    return 0;
}
