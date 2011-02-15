#ifndef PLAYER_H
#define PLAYER_H

enum ActionType {
	PRESS,
	FLAG,
	GIVEUP
};

typedef struct {
	enum ActionType type;
	int tileidx;
} Action;

class Minefield;
template <class ConcretePlayer> class Player {
	public:
		void init(Minefield *f) {
			static_cast<ConcretePlayer*>(this)->init(f);
		}
		void deinit(Minefield *f) {
			static_cast<ConcretePlayer*>(this)->deinit(f);
		}
		Action **act(Minefield *f) {
			return static_cast<ConcretePlayer*>(this)->act(f);
		}
		void free(Action **act) {
			static_cast<ConcretePlayer*>(this)->free(act);
		}
};

#endif
