/*
** deltalib.c
** Delta Lua API - stub implementation for emulator integration.
**
** This file intentionally provides "do-nothing" implementations that:
** - Validate/parse Lua arguments
** - Return sane placeholder values
** - Provide clear error messages for unsupported operations
**
** The authoritative API reference is attachments/20260112_021517_delta_api.txt.
**
** IMPORTANT: These are stubs. Integrate with your emulator/ROS2 runtime by
** replacing the placeholder behavior in each function.
*/

#define deltalib_c
#define LUA_LIB

#include "lprefix.h"

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"

/* ----------------------------- Shared helpers ----------------------------- */

/* Delta uses IO status words ON/OFF in docs. We accept:
 * - boolean
 * - strings "ON"/"OFF" (case-insensitive for common variants)
 * - numbers: 0 = OFF, non-zero = ON
 */
static int delta_check_onoff(lua_State *L, int idx) {
  if (lua_isboolean(L, idx)) return lua_toboolean(L, idx) ? 1 : 0;
  if (lua_isnumber(L, idx)) return (lua_tonumber(L, idx) != 0) ? 1 : 0;
  if (lua_isstring(L, idx)) {
    size_t len = 0;
    const char *s = lua_tolstring(L, idx, &len);
    if (s == NULL) luaL_argerror(L, idx, "status must be ON/OFF, boolean, or number");
    /* handle typical inputs: "ON", "OFF" */
    if ((len == 2 && (s[0] == 'O' || s[0] == 'o') && (s[1] == 'N' || s[1] == 'n')) ||
        (len == 4 && (s[0] == 'T' || s[0] == 't') && (s[1] == 'R' || s[1] == 'r') &&
         (s[2] == 'U' || s[2] == 'u') && (s[3] == 'E' || s[3] == 'e')))
      return 1;
    if ((len == 3 && (s[0] == 'O' || s[0] == 'o') && (s[1] == 'F' || s[1] == 'f') &&
         (s[2] == 'F' || s[2] == 'f')) ||
        (len == 5 && (s[0] == 'F' || s[0] == 'f') && (s[1] == 'A' || s[1] == 'a') &&
         (s[2] == 'L' || s[2] == 'l') && (s[3] == 'S' || s[3] == 's') &&
         (s[4] == 'E' || s[4] == 'e')))
      return 0;

    luaL_argerror(L, idx, "status string must be ON or OFF");
  }
  luaL_argerror(L, idx, "status must be ON/OFF, boolean, number, or string");
  return 0;
}

static int delta_opt_int(lua_State *L, int idx, int dflt) {
  if (lua_isnoneornil(L, idx)) return dflt;
  return (int)luaL_checkinteger(L, idx);
}

static lua_Number delta_opt_number(lua_State *L, int idx, lua_Number dflt) {
  if (lua_isnoneornil(L, idx)) return dflt;
  return luaL_checknumber(L, idx);
}

static int delta_is_valid_name(const char *s, size_t len) {
  /* docs allow English, numbers, symbols (@ _ ! # +), max length 16.
     We'll only enforce length here in stub. */
  (void)s;
  return (len > 0 && len <= 16);
}

/* Accept Pin_Index as number or name string. Return number if numeric, else keep string. */
static void delta_check_pin_index(lua_State *L, int idx, int min, int max) {
  if (lua_isinteger(L, idx) || lua_isnumber(L, idx)) {
    int pin = (int)luaL_checkinteger(L, idx);
    luaL_argcheck(L, pin >= min && pin <= max, idx, "pin index out of range");
  } else if (lua_isstring(L, idx)) {
    size_t len = 0;
    const char *name = luaL_checklstring(L, idx, &len);
    luaL_argcheck(L, delta_is_valid_name(name, len), idx, "pin name must be 1..16 chars");
  } else {
    luaL_argerror(L, idx, "pin index must be a number or string");
  }
}

/* ---------------------------- DI/DO primitives ---------------------------- */

/*
### function DI ProtoType
Status = DI(Pin_Index)
Status_num = DI(Pin_Index, Length)
Pin_Index: number(1..24) or name string
Length: number(1..24)
Returns:
 - Status: "ON"/"OFF" (word)
 - Status_num: number (bitfield)
*/
static int l_delta_DI(lua_State *L) {
  int nargs = lua_gettop(L);
  delta_check_pin_index(L, 1, 1, 24);

  if (nargs == 1) {
    /* Stub: always OFF */
    lua_pushstring(L, "OFF");
    return 1;
  } else {
    int length = (int)luaL_checkinteger(L, 2);
    luaL_argcheck(L, length >= 1 && length <= 24, 2, "length must be 1..24");

    /* Stub: all OFF -> 0 */
    lua_pushinteger(L, 0);
    return 1;
  }
}

/*
### Function DO Prototype
DO(Pin_Index, Status)
DO(Pin_Index, Status, Delay_Time)
DO(Pin_Index, Length, Status_num)
DO(Pin_Index, Length, Status_num, Delay_Time)
Pin_Index: number(1..12) or name string
Status: ON/OFF
Delay_Time: seconds (number)
Length: number(1..12)
Status_num: number bitfield
Returns: nothing
*/
static int l_delta_DO(lua_State *L) {
  int nargs = lua_gettop(L);
  delta_check_pin_index(L, 1, 1, 12);

  if (nargs < 2) {
    return luaL_error(L, "DO expects at least 2 arguments");
  }

  /* Overload resolution:
     - If arg2 is integer and arg3 exists and is number: treat as (Pin_Index, Length, Status_num, [Delay_Time])
     - Else: treat as (Pin_Index, Status, [Delay_Time])
  */
  if (lua_isinteger(L, 2) && (nargs >= 3) && lua_isnumber(L, 3)) {
    int length = (int)luaL_checkinteger(L, 2);
    lua_Integer status_num = luaL_checkinteger(L, 3);
    lua_Number delay = (nargs >= 4) ? luaL_checknumber(L, 4) : 0.0;

    luaL_argcheck(L, length >= 1 && length <= 12, 2, "length must be 1..12");
    luaL_argcheck(L, delay >= 0.0, 4, "delay time must be >= 0");

    (void)status_num; /* stub */
  } else {
    int status = delta_check_onoff(L, 2);
    lua_Number delay = (nargs >= 3) ? luaL_checknumber(L, 3) : 0.0;
    luaL_argcheck(L, delay >= 0.0, 3, "delay time must be >= 0");
    (void)status; /* stub */
  }

  return 0;
}

/*
### Function ExtDI Prototype
Status = ExtDI(Address_Index, Pin_Index)
Address_Index: number (station number)
Pin_Index: number (range board-specific)
Returns: "ON"/"OFF"
*/
static int l_delta_ExtDI(lua_State *L) {
  (void)luaL_checkinteger(L, 1); /* station */
  (void)luaL_checkinteger(L, 2); /* pin */
  lua_pushstring(L, "OFF");
  return 1;
}

/*
### Function ExtDO Prototype
ExtDO(Address_Index, Pin_Index, Status)
ExtDO(Address_Index, Pin_Index, Status, Delay_Time)
Returns: nothing
*/
static int l_delta_ExtDO(lua_State *L) {
  int nargs = lua_gettop(L);
  (void)luaL_checkinteger(L, 1);
  (void)luaL_checkinteger(L, 2);
  (void)delta_check_onoff(L, 3);
  if (nargs >= 4) {
    lua_Number delay = luaL_checknumber(L, 4);
    luaL_argcheck(L, delay >= 0.0, 4, "delay time must be >= 0");
  }
  return 0;
}

/* ----------------------------- Motion stubs ------------------------------ */

/*
### Function MovP Prototype
MovP(Point)
MovP(Point + Offset(), Function()+...)
Point: string or number
(Offset/Function parsing is not implemented in this stub.)
*/
static int l_delta_MovP(lua_State *L) {
  luaL_checkany(L, 1); /* point id */
  /* Stub: accept but do nothing */
  return 0;
}

/*
### Function MovL Prototype (doc typo says MovP, but content indicates MovL)
MovL(Point)
MovL(Point + Offset(), Function()+...)
*/
static int l_delta_MovL(lua_State *L) {
  luaL_checkany(L, 1);
  return 0;
}

/*
### Function MovJ Prototype
MovJ(Joint, Degree, Function()+...)
Joint: 1..6
Degree: number
*/
static int l_delta_MovJ(lua_State *L) {
  int joint = (int)luaL_checkinteger(L, 1);
  lua_Number degree = luaL_checknumber(L, 2);
  luaL_argcheck(L, joint >= 1 && joint <= 6, 1, "joint must be 1..6");
  (void)degree;
  return 0;
}

/*
### Function SpdJ Prototype
SpdJ(Speed)  Speed: 0.001..100 (%)
*/
static int l_delta_SpdJ(lua_State *L) {
  lua_Number spd = luaL_checknumber(L, 1);
  luaL_argcheck(L, spd > 0.0 && spd <= 100.0, 1, "speed must be in (0, 100]");
  return 0;
}

/*
### Function AccJ Prototype
AccJ(Acceleration) 0.001..100 (%)
*/
static int l_delta_AccJ(lua_State *L) {
  lua_Number acc = luaL_checknumber(L, 1);
  luaL_argcheck(L, acc > 0.0 && acc <= 100.0, 1, "acceleration must be in (0, 100]");
  return 0;
}

/*
### Function DecJ Prototype
DecJ(Deceleration) 0.001..100 (%)
*/
static int l_delta_DecJ(lua_State *L) {
  lua_Number dec = luaL_checknumber(L, 1);
  luaL_argcheck(L, dec > 0.0 && dec <= 100.0, 1, "deceleration must be in (0, 100]");
  return 0;
}

/*
### Function SpdL Prototype
SpdL(Speed) Speed: 1..2000 (mm/sec)
*/
static int l_delta_SpdL(lua_State *L) {
  lua_Integer spd = luaL_checkinteger(L, 1);
  luaL_argcheck(L, spd >= 1 && spd <= 2000, 1, "speed must be 1..2000");
  return 0;
}

/*
### Function AccL Prototype
AccL(Acceleration) 1..25000 (mm/sec^2)
*/
static int l_delta_AccL(lua_State *L) {
  lua_Integer acc = luaL_checkinteger(L, 1);
  luaL_argcheck(L, acc >= 1 && acc <= 25000, 1, "acceleration must be 1..25000");
  return 0;
}

/*
### Function DecL Prototype
DecL(Deceleration) 1..25000 (mm/sec^2)
*/
static int l_delta_DecL(lua_State *L) {
  lua_Integer dec = luaL_checkinteger(L, 1);
  luaL_argcheck(L, dec >= 1 && dec <= 25000, 1, "deceleration must be 1..25000");
  return 0;
}

/*
### Function Accur Prototype
Accur(Mode, "CART")
Mode: string HIGH|STANDARD|MEDIUM|ROUGH|MAXROUGH
Second arg is string "CART" per doc.
*/
static int l_delta_Accur(lua_State *L) {
  const char *mode = luaL_checkstring(L, 1);
  const char *typ = luaL_checkstring(L, 2);

  luaL_argcheck(L, strcmp(typ, "CART") == 0, 2, "second argument must be \"CART\"");

  /* Stub: just validate mode */
  if (strcmp(mode, "HIGH") && strcmp(mode, "STANDARD") && strcmp(mode, "MEDIUM") &&
      strcmp(mode, "ROUGH") && strcmp(mode, "MAXROUGH")) {
    return luaL_error(L, "unknown accuracy mode: %s", mode);
  }

  return 0;
}

/*
### Function SetGlobalPoint Prototype
Four-axis: SetGlobalPoint(Point, PointName, X, Y, Z, RZ, Hand, UF, TF, JRC)
Five-axis: SetGlobalPoint(Point, PointName, X, Y, Z, RY, RZ, Hand, UF, TF, JRC)
Six-axis:  SetGlobalPoint(Point, PointName, X, Y, Z, RX, RY, RZ, Elbow, Shoulder, Flip, UF, TF, JRC)
We accept variable arity and validate minimum common parts.
*/
static int l_delta_SetGlobalPoint(lua_State *L) {
  int nargs = lua_gettop(L);
  lua_Integer point = luaL_checkinteger(L, 1);
  luaL_argcheck(L, point >= 1 && point <= 1000, 1, "point must be 1..1000");

  /* PointName optional per doc; but signature shows it present. We'll allow nil or string. */
  if (!lua_isnoneornil(L, 2)) {
    const char *pname = luaL_checkstring(L, 2);
    /* doc says prefix "GL_" */
    if (strncmp(pname, "GL_", 3) != 0) {
      /* do not hard fail; warn by allowing, but strictness could be enforced later */
    }
  }

  luaL_argcheck(L, nargs >= 6, 1, "SetGlobalPoint expects at least 6 arguments (type-dependent)");
  /* Remaining args are accepted; stub does nothing */
  return 0;
}

/* --------------------------- WAIT / Modbus stubs -------------------------- */

/*
### Function WAIT Prototype
WAIT(IO type, IO index/Multi IO index, DI/DO status, Timeout)
WAIT(Modbus variable, Modbus address, Modbus data type, Modbus data)
We detect by arg1 being string "DI"/"DO" for IO-wait; otherwise treat as Modbus wait.
*/
static int l_delta_WAIT(lua_State *L) {
  int nargs = lua_gettop(L);
  luaL_argcheck(L, nargs >= 4, 1, "WAIT expects 4 arguments");

  if (lua_isstring(L, 1)) {
    const char *io_type = lua_tostring(L, 1);
    if (io_type && (!strcmp(io_type, "DI") || !strcmp(io_type, "DO"))) {
      /* IO wait */
      /* arg2 may be number or table; accept anything in stub */
      (void)luaL_checkany(L, 2);
      (void)delta_check_onoff(L, 3);
      lua_Integer timeout_ms = luaL_checkinteger(L, 4);
      luaL_argcheck(L, timeout_ms >= 0, 4, "timeout must be >= 0 (ms)");
      return 0;
    }
  }

  /* Modbus wait */
  (void)luaL_checkany(L, 1);              /* Modbus variable (word) */
  (void)luaL_checkany(L, 2);              /* address number or table */
  const char *dtype = luaL_checkstring(L, 3); /* "W" or "DW" */
  luaL_argcheck(L, !strcmp(dtype, "W") || !strcmp(dtype, "DW"), 3, "data type must be \"W\" or \"DW\"");
  (void)luaL_checknumber(L, 4); /* Modbus data */
  return 0;
}

/*
### Function ReadModbus prototype
data = ReadModbus(RegAddress, Size)
Size: "W" or "DW"
Returns: number
*/
static int l_delta_ReadModbus(lua_State *L) {
  lua_Integer addr = luaL_checkinteger(L, 1);
  const char *size = luaL_checkstring(L, 2);
  luaL_argcheck(L, addr >= 0, 1, "RegAddress must be >= 0");
  luaL_argcheck(L, !strcmp(size, "W") || !strcmp(size, "DW"), 2, "Size must be \"W\" or \"DW\"");

  /* Stub: return 0 */
  lua_pushinteger(L, 0);
  return 1;
}

/*
### Function WriteModbus prototype
WriteModbus(RegAddress, Size, RegValue)
*/
static int l_delta_WriteModbus(lua_State *L) {
  lua_Integer addr = luaL_checkinteger(L, 1);
  const char *size = luaL_checkstring(L, 2);
  (void)luaL_checknumber(L, 3);
  luaL_argcheck(L, addr >= 0, 1, "RegAddress must be >= 0");
  luaL_argcheck(L, !strcmp(size, "W") || !strcmp(size, "DW"), 2, "Size must be \"W\" or \"DW\"");
  return 0;
}

/* ---------------------------- SocketClass stub ---------------------------- */

typedef struct DeltaSocket {
  int is_open;
  char host[64];
  int port;
  char spacing;   /* delimiter for splitting receive into array */
  char delimiter; /* end symbol appended on send */
  char cmd[128];
  lua_Number sleeptime;
  lua_Number timeout;
} DeltaSocket;

static DeltaSocket *delta_check_socket(lua_State *L, int idx) {
  return (DeltaSocket *)luaL_checkudata(L, idx, "Delta.SocketClass");
}

/*
### SocketClass
Variable = SocketClass(Host IP, Port, Spacing, Delimiter, Cmd, Sleeptime, Timeout)
Spacing: char or nil (default ',')
Delimiter: char or nil (default '\r\n') in doc, but that's 2 chars; we store first char for stub.
Cmd: string or nil
Sleeptime: number or nil (default 0.1)
Timeout: number (default 10)
Returns: userdata with methods Send/Receive/Close.
*/
static int l_delta_SocketClass(lua_State *L) {
  const char *host = luaL_checkstring(L, 1);
  lua_Integer port = luaL_checkinteger(L, 2);

  /* spacing (arg3) is 'char' or nil */
  char spacing = ',';
  if (!lua_isnoneornil(L, 3)) {
    size_t slen = 0;
    const char *s = luaL_checklstring(L, 3, &slen);
    luaL_argcheck(L, slen >= 1, 3, "Spacing must be a character");
    spacing = s[0];
  }

  /* delimiter (arg4) is 'char' or nil, doc mentions default '\r\n' */
  char delimiter = '\n';
  if (!lua_isnoneornil(L, 4)) {
    size_t dlen = 0;
    const char *d = luaL_checklstring(L, 4, &dlen);
    luaL_argcheck(L, dlen >= 1, 4, "Delimiter must be a character");
    delimiter = d[0];
  }

  const char *cmd = NULL;
  if (!lua_isnoneornil(L, 5)) cmd = luaL_checkstring(L, 5);

  lua_Number sleeptime = delta_opt_number(L, 6, 0.1);
  lua_Number timeout = delta_opt_number(L, 7, 10.0);

  luaL_argcheck(L, port > 0 && port <= 65535, 2, "Port must be 1..65535");
  luaL_argcheck(L, sleeptime >= 0.0, 6, "Sleeptime must be >= 0");
  luaL_argcheck(L, timeout > 0.0, 7, "Timeout must be > 0");

  DeltaSocket *sock = (DeltaSocket *)lua_newuserdatauv(L, sizeof(DeltaSocket), 0);
  memset(sock, 0, sizeof(*sock));
  sock->is_open = 1;
  strncpy(sock->host, host, sizeof(sock->host) - 1);
  sock->port = (int)port;
  sock->spacing = spacing;
  sock->delimiter = delimiter;
  if (cmd) strncpy(sock->cmd, cmd, sizeof(sock->cmd) - 1);
  sock->sleeptime = sleeptime;
  sock->timeout = timeout;

  luaL_getmetatable(L, "Delta.SocketClass");
  lua_setmetatable(L, -2);
  return 1;
}

/*
### Function Send Prototype
Variable:Send(Cmd)
Cmd: string/number/variable
Returns: receiver variable (doc shows "Variable : Send(Cmd)" - unclear; we'll return self)
*/
static int l_delta_socket_Send(lua_State *L) {
  DeltaSocket *sock = delta_check_socket(L, 1);
  luaL_argcheck(L, sock->is_open, 1, "socket is closed");
  luaL_checkany(L, 2); /* cmd */
  lua_settop(L, 1);
  return 1;
}

/*
### Function Receive Prototype
ret = Variable:Receive()
Returns: string or table (if delimiter split)
Stub returns empty string.
*/
static int l_delta_socket_Receive(lua_State *L) {
  DeltaSocket *sock = delta_check_socket(L, 1);
  luaL_argcheck(L, sock->is_open, 1, "socket is closed");

  /* Stub: no real data */
  lua_pushliteral(L, "");
  return 1;
}

/*
### Function Close Prototype
Variable:Close()
*/
static int l_delta_socket_Close(lua_State *L) {
  DeltaSocket *sock = delta_check_socket(L, 1);
  sock->is_open = 0;
  return 0;
}

static int l_delta_socket___gc(lua_State *L) {
  DeltaSocket *sock = delta_check_socket(L, 1);
  sock->is_open = 0;
  return 0;
}

static int l_delta_socket___tostring(lua_State *L) {
  DeltaSocket *sock = delta_check_socket(L, 1);
  lua_pushfstring(L, "SocketClass(%s:%d)%s", sock->host, sock->port,
                  sock->is_open ? "" : " [closed]");
  return 1;
}

/* ------------------------------- Lua module ------------------------------- */

static const luaL_Reg deltaLib[] = {
  /* IO */
  {"DI", l_delta_DI},
  {"DO", l_delta_DO},
  {"ExtDI", l_delta_ExtDI},
  {"ExtDO", l_delta_ExtDO},

  /* Motion / config */
  {"MovP", l_delta_MovP},
  {"MovL", l_delta_MovL},
  {"MovJ", l_delta_MovJ},
  {"SetGlobalPoint", l_delta_SetGlobalPoint},
  {"SpdJ", l_delta_SpdJ},
  {"AccJ", l_delta_AccJ},
  {"DecJ", l_delta_DecJ},
  {"SpdL", l_delta_SpdL},
  {"AccL", l_delta_AccL},
  {"DecL", l_delta_DecL},
  {"Accur", l_delta_Accur},

  /* Wait / Modbus */
  {"WAIT", l_delta_WAIT},
  {"ReadModbus", l_delta_ReadModbus},
  {"WriteModbus", l_delta_WriteModbus},

  /* SocketClass constructor */
  {"SocketClass", l_delta_SocketClass},

  {NULL, NULL}
};

static void delta_create_socket_metatable(lua_State *L) {
  if (luaL_newmetatable(L, "Delta.SocketClass")) {
    /* methods */
    luaL_Reg methods[] = {
      {"Send", l_delta_socket_Send},
      {"Receive", l_delta_socket_Receive},
      {"Close", l_delta_socket_Close},
      {"__gc", l_delta_socket___gc},
      {"__tostring", l_delta_socket___tostring},
      {NULL, NULL}
    };

    luaL_setfuncs(L, methods, 0);

    /* __index = metatable */
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
  }
  lua_pop(L, 1);
}

/* PUBLIC_INTERFACE */
LUAMOD_API int luaopen_delta(lua_State *L) {
  /**
   * Lua module entrypoint for the Delta API stubs.
   *
   * Returns:
   *  - a table containing functions matching delta_api.txt (DI/DO/Mov*/Spd*/Acc*/...),
   *    plus SocketClass userdata constructor.
   */
  delta_create_socket_metatable(L);
  luaL_newlib(L, deltaLib);
  return 1;
}
