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
		void init(Minefield *f) {CALL(init, f);}
		void deinit(Minefield *f) {CALL(deinit, f);}
		void updatefield(Minefield *f, const char *field) {CALL(updatefield, f, field);}
		void updatetile(Minefield *f, int idx) {CALL(updatetile, f, idx);}
		void vspeak(Minefield *f, const char *fmt, va_list args) {CALL(vspeak, f, fmt, args);}
		void mark(Minefield *f, int idx, int mark) {CALL(mark, f, idx, mark);}
		void resetmarks(Minefield *f) {CALL(resetmarks, f);}
		void speak(Minefield *f, const char *fmt, ...) {
			va_list argp;
			va_start(argp, fmt);
			CALL(vspeak, f, fmt, argp);
			va_end(argp);
		}
};

#endif
