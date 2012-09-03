extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
};
#include <stdio.h>

template <class T>
class LunaWrapper
{
public:
	static void Register(lua_State *L)
	{
		lua_pushcfunction(L, LunaWrapper<T>::Constructor);
		lua_setglobal(L, T::ClassName);

		luaL_newmetatable(L, T::ClassName);
		lua_pushstring(L, "__gc");
		lua_pushcfunction(L, LunaWrapper<T>::Destructor);
		lua_settable(L, -3);
		return;
	}

	static int Constructor(lua_State *L)
	{
		lua_newtable(L);
		lua_pushnumber(L, 0);
		// TODO: parse args, then construct T obj
		T *pObj = new T();
		T **ppObj = (T **)lua_newuserdata(L, sizeof(T *));
		*ppObj = pObj;
		luaL_getmetatable(L, T::ClassName);
		lua_setmetatable(L, -2);

		lua_settable(L, -3); // table[0] = obj
		for (int i = 0; T::Register[i].name; i++)
		{
			lua_pushstring(L, T::Register[i].name);
			lua_pushnumber(L, i);
			lua_pushcclosure(L, &LunaWrapper<T>::Thunk, 1);
			lua_settable(L, -3);
		}
		return 1;
	}

	static int Thunk(lua_State *L)
	{
		int i = (int)lua_tonumber(L, lua_upvalueindex(1));
		lua_pushnumber(L, 0);
		lua_gettable(L, 1);
		T **ppObj = static_cast<T **>(luaL_checkudata(L, -1, T::ClassName));
		lua_remove(L, -1);
		return ((*ppObj)->*T::Register[i].func)(L);
	}

	static int Destructor(lua_State *L)
	{
		T **ppObj = static_cast<T **>(luaL_checkudata(L, -1, T::ClassName));
		delete *ppObj;
		return 0;
	}

	struct RegType
	{
		const char *name;
		int (T::*func)(lua_State *);
	};
};

class Foo
{
	public:
	static const char *ClassName;
	static LunaWrapper<Foo>::RegType Register[];

	Foo() { printf("Foo()\n"); }
	~Foo() { printf("~Foo()\n"); }
	int testRegisterFunc(lua_State *L)
	{
		/*
		 * 1:arg1 ... n:argn
		 */
		printf("testRegisterFunc(lua_State *)\n");
		return 0;
	}
};

const char * Foo::ClassName = "Foo";
LunaWrapper<Foo>::RegType Foo::Register[] =
{
	{"testRegisterFunc", &Foo::testRegisterFunc},
	{NULL, NULL},
};

int main()
{
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	LunaWrapper<Foo>::Register(L);
	if (luaL_dofile(L, "test.lua"))
		printf("%s\n", lua_tostring(L, -1));
	lua_close(L);
	return 0;
}
