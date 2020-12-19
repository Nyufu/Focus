#include <type_traits.hxx>
#include <types.hxx>
#include <abi/x64.hxx>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ABI::X64 {

#define FENCE() if (IsDebuggerPresent()) __debugbreak()

enum class AnnotationResult {
	NOT_ANNOTATED,
	BY_REGISTER,
	BY_ARG
};

std::wstring ToString(const AnnotationResult*) { return L""; }
std::wstring ToString(const AnnotationResult&) { return L""; }
std::wstring ToString(AnnotationResult*) { return L""; }

template<class Type, AnnotationResult annotation>
__declspec(noinline) void TraitCheck() {
	static_assert((annotation == AnnotationResult::BY_REGISTER) ==
		abi::x64::is_return_by_register_v<Type> ||
		annotation == AnnotationResult::NOT_ANNOTATED, "is_trivially_returnable_v trait doesn't work with <Type>");
	Assert::IsTrue(AnnotationResult::NOT_ANNOTATED != annotation, L"Type isn't annotated");
}

TEST_CLASS(IsTriviallyReturnable) {
public:

	/*
	TEST_METHOD(Template) {
		struct S {
		};

		using Type = S;

		static_assert(abi::x64::is_definable_return_policy_v<Type>);

		struct {
			static Type Func() {
				return {};
			}
		} obj;

		FENCE();
		[[maybe_unused]] auto result = obj.Func();

		TraitCheck<Type, AnnotationResult::NOT_ANNOTATED>();
	}

	TEST_METHOD(Template) {
		struct S {
			S() = default;
		};

		using Type = S;

		static_assert(!abi::x64::is_definable_return_policy_v<Type>);
	}
	*/

	TEST_METHOD(Void) {
		using Type = void;

		static_assert(abi::x64::is_definable_return_policy_v<Type>);

		struct {
			static Type Func() {
				return;
			}
		} obj;

		FENCE();
		obj.Func();

		TraitCheck<Type, AnnotationResult::BY_REGISTER>();
	}

	TEST_METHOD(Int) {
		using Type = int;

		static_assert(abi::x64::is_definable_return_policy_v<Type>);

		struct {
			static Type Func() {
				return {};
			}
		} obj;

		FENCE();
		[[maybe_unused]] auto result = obj.Func();

		TraitCheck<Type, AnnotationResult::BY_REGISTER>();
	}

	TEST_METHOD(EmptyType) {
		struct S {
		};

		using Type = S;

		static_assert(abi::x64::is_definable_return_policy_v<Type>);

		struct {
			static Type Func() {
				return {};
			}
		} obj;

		FENCE();
		[[maybe_unused]] auto result = obj.Func();

		TraitCheck<Type, AnnotationResult::BY_REGISTER>();
	}

	TEST_METHOD(OnePublicIntMember) {
		struct S {
			int i;
		};

		using Type = S;

		static_assert(abi::x64::is_definable_return_policy_v<Type>);

		struct {
			static Type Func() {
				return {};
			}
		} obj;

		FENCE();
		[[maybe_unused]] auto result = obj.Func();

		TraitCheck<Type, AnnotationResult::BY_REGISTER>();
	}

	TEST_METHOD(OnePrivateIntMember) {
		struct S {
		private:
			int i;
		};

		using Type = S;

		static_assert(!abi::x64::is_definable_return_policy_v<Type>);
	}

	TEST_METHOD(UserDefaultedDefaultConstructor) {
		struct S {
			S() = default;
		};

		using Type = S;

		static_assert(!abi::x64::is_definable_return_policy_v<Type>);
	}

	TEST_METHOD(UserProvidedDefaultConstructor) {
		struct S {
			S() {}
		};

		using Type = S;

		static_assert(abi::x64::is_definable_return_policy_v<Type>);

		struct {
			static Type Func() {
				return {};
			}
		} obj;

		FENCE();
		[[maybe_unused]] auto result = obj.Func();

		TraitCheck<Type, AnnotationResult::BY_ARG>();
	}

	TEST_METHOD(UserDefaultedDefaultConstructorOnePrivateInt) {
		struct S {
			S() = default;
		private:
			int i;
		};

		using Type = S;

		static_assert(!abi::x64::is_definable_return_policy_v<Type>);
	}

	TEST_METHOD(UserProvidedDefaultConstructorOnePrivateInt) {
		struct S {
			S() {}
		private:
			int i;
		};

		using Type = S;

		static_assert(abi::x64::is_definable_return_policy_v<Type>);

		struct {
			static Type Func() {
				return {};
			}
		} obj;

		FENCE();
		[[maybe_unused]] auto result = obj.Func();

		TraitCheck<Type, AnnotationResult::BY_ARG>();
	}

	TEST_METHOD(Derived) {
		struct SB {
		};
		struct S : SB {
		};

		using Type = S;

		static_assert(abi::x64::is_definable_return_policy_v<Type>);

		struct {
			static Type Func() {
				return {};
			}
		} obj;

		FENCE();
		[[maybe_unused]] auto result = obj.Func();

		TraitCheck<Type, AnnotationResult::BY_ARG>();
	}

	TEST_METHOD(DerivedUserProvidedDefaultConstructor) {
		struct SB {
		};
		struct S : SB {
			S() {}
		};

		using Type = S;

		static_assert(abi::x64::is_definable_return_policy_v<Type>);

		struct {
			static Type Func() {
				return {};
			}
		} obj;

		FENCE();
		[[maybe_unused]] auto result = obj.Func();

		TraitCheck<Type, AnnotationResult::BY_ARG>();
	}

	TEST_METHOD(DerivedCompilerGeneratedConstructor) {
		struct SB {
			SB() {}
		};
		struct S : SB {
		};

		using Type = S;

		std::is_aggregate_v<Type>;

		static_assert(abi::x64::is_definable_return_policy_v<Type>);

		struct {
			static Type Func() {
				return {};
			}
		} obj;

		FENCE();
		[[maybe_unused]] auto result = obj.Func();

		TraitCheck<Type, AnnotationResult::BY_ARG>();
	}

	TEST_METHOD(DerivedDefaultedDefaultConstructor) {
		struct SB {
		};
		struct S : SB {
			S() = default;
		};

		using Type = S;

		static_assert(!abi::x64::is_definable_return_policy_v<Type>);
	}

	TEST_METHOD(VirtaulDefaultDestructor) {
		struct S {
			virtual ~S() = default;
		};

		using Type = S;

		static_assert(abi::x64::is_definable_return_policy_v<Type>);

		struct {
			static Type Func() {
				return {};
			}
		} obj;

		FENCE();
		[[maybe_unused]] auto result = obj.Func();

		TraitCheck<Type, AnnotationResult::BY_ARG>();
	}

	TEST_METHOD(VirtualFunction) {
		struct S {
			virtual void foo() {}
		};

		using Type = S;

		static_assert(abi::x64::is_definable_return_policy_v<Type>);

		struct {
			static Type Func() {
				return {};
			}
		} obj;

		FENCE();
		[[maybe_unused]] auto result = obj.Func();

		TraitCheck<Type, AnnotationResult::BY_ARG>();
	}

	TEST_METHOD(UserProvidedConstructors) {
		struct S {
			S() = default;
			S(int) {
			}
			int i;
		};

		using Type = S;

		static_assert(!abi::x64::is_definable_return_policy_v<Type>);
	}

	TEST_METHOD(UserProvidedNonDefaultConstructor) {
		struct S {
			S(int) {
			}
		};

		using Type = S;

		static_assert(abi::x64::is_definable_return_policy_v<Type>);

		struct {
			static Type Func() {
				return { 0 };
			}
		} obj;

		FENCE();
		[[maybe_unused]] auto result = obj.Func();

		TraitCheck<Type, AnnotationResult::BY_ARG>();
	}


	TEST_METHOD(ReferenceMember) {
		struct S {
			int& i;
		};

		using Type = S;

		static_assert(abi::x64::is_definable_return_policy_v<Type>);

		struct {
			static Type Func() {
				static int i = 0;
				return { i };
			}
		} obj;

		FENCE();
		[[maybe_unused]] auto result = obj.Func();

		TraitCheck<Type, AnnotationResult::BY_ARG>();
	}

	TEST_METHOD(IntRef) {
		using Type = int&;

		static_assert(abi::x64::is_definable_return_policy_v<Type>);

		struct {
			static Type Func() {
				static int i = 0;
				return { i };
			}
		} obj;

		FENCE();
		[[maybe_unused]] auto result = obj.Func();

		TraitCheck<Type, AnnotationResult::BY_REGISTER>();
	}

	TEST_METHOD(NonPODMember) {
		struct C {
			C() = delete;
			C(int i_)
				: i{ i_ } {
			}
			int i;
		};
		struct S {
			C c;
		};

		using Type = S;

		static_assert(abi::x64::is_definable_return_policy_v<Type>);

		struct {
			static Type Func() {
				return { 0 };
			}
		} obj;

		FENCE();
		[[maybe_unused]] auto result = obj.Func();

		TraitCheck<Type, AnnotationResult::BY_REGISTER>();
	}

	TEST_METHOD(PODWithRefMember) {
		struct C {
			int& i;
		};
		struct S {
			C c;
		};
		using Type = S;

		static_assert(abi::x64::is_definable_return_policy_v<Type>);

		struct {
			static Type Func() {
				int i = 0;
				return { i };
			}
		} obj;

		FENCE();
		[[maybe_unused]] auto result = obj.Func();

		TraitCheck<Type, AnnotationResult::BY_ARG>();
	}

	TEST_METHOD(BigPOD) {
		struct S {
			long long x;
			long long y;
		};

		using Type = S;

		static_assert(abi::x64::is_definable_return_policy_v<Type>);

		struct {
			static Type Func() {
				return {};
			}
		} obj;

		FENCE();
		[[maybe_unused]] auto result = obj.Func();

		TraitCheck<Type, AnnotationResult::BY_ARG>();
	}

	TEST_METHOD(Enum) {
		enum enum_
		{
			A,
			B
		};
		using Type = enum_;

		static_assert(abi::x64::is_definable_return_policy_v<Type>);

		struct {
			static Type Func() {
				return Type::A;
			}
		} obj;

		FENCE();
		[[maybe_unused]] auto result = obj.Func();

		TraitCheck<Type, AnnotationResult::BY_REGISTER>();
	}

	TEST_METHOD(EnumClass) {
		enum class enum_class
		{
			A,
			B
		};
		using Type = enum_class;

		static_assert(abi::x64::is_definable_return_policy_v<Type>);

		struct {
			static Type Func() {
				return Type::A;
			}
		} obj;

		FENCE();
		[[maybe_unused]] auto result = obj.Func();

		TraitCheck<Type, AnnotationResult::BY_REGISTER>();
	}
};

}

