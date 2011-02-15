#ifndef AI_H
#define AI_H
#include "Player.h"
#include "cmines.h"

class AI : public Player<AI> {
	public:
		AI(Minefield *);
		void init(Minefield *);
		void deinit(Minefield *);
		Action ** act(Minefield *);
		void free(Action **);

	private:
		bool allowcoordreset;
		int nexttileidx_;

		void giveup(Action *);
		bool hasnexttile(Minefield *);
		int nexttileidx(Minefield *);

		void filterunknown(Minefield *, int *);
		int countunknown(Minefield *, int *);
		int countflags(Minefield *, int *);
		void neighbourdifference(Minefield *, int *, int *);

		bool issubset(int *superset, int *subset, int length);
		Action **act_singlecheck(Minefield *, int idx);
		Action **act_dualcheck(Minefield *, int idx);
};

#endif
