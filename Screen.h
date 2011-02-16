#ifndef SCREEN_H
#define SCREEN_H

#include <stdarg.h>

#define SCREENMARK_PRIMARY (1)
#define SCREENMARK_SECONDARY (2)

typedef struct {
	int idx;
	int mark;
} Mark;

#define CALL(fn, ...) static_cast<ConcreteScreen*>(this)->fn(__VA_ARGS__)
class Minefield;
template <class ConcreteScreen> class Screen {
	public:
		void init() {CALL(init);}
		void deinit() {CALL(deinit);}
		void updatefield(const char *field) {CALL(updatefield, field);}
		void updatetile(int idx) {CALL(updatetile, idx);}
		void vspeak(const char *fmt, va_list args) {CALL(vspeak, fmt, args);}
		void mark(int idx, int mark) {CALL(mark, idx, mark);}
		void resetmarks() {CALL(resetmarks);}
		void speak(const char *fmt, ...) {
			va_list argp;
			va_start(argp, fmt);
			CALL(vspeak, fmt, argp);
			va_end(argp);
		}
};

#endif
