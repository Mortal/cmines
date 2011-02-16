#ifndef PLAYER_H
#define PLAYER_H

enum ActionType {
	PRESS,
	FLAG,
	GIVEUP,
	NOOP
};

typedef struct {
	enum ActionType type;
	int tileidx;
} Action;

class Minefield;
template <class ConcretePlayer> class Player {
	public:
		Action **act() {
			return static_cast<ConcretePlayer*>(this)->act();
		}
		void free(Action **act) {
			static_cast<ConcretePlayer*>(this)->free(act);
		}
};

#endif
