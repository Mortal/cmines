#ifndef AI_H
#define AI_H
#include "Player.h"
#include "Minefield.h"

class AI : public Player<AI> {
	public:
		AI(Minefield *);
		Action ** act();
		void free(Action **);

	private:
		Minefield *f;

		bool allowcoordreset;
		int nexttileidx_;

		void giveup(Action *);
		bool hasnexttile();
		int nexttileidx();

		template<typename CB>
		static int counter(Minefield * f, int * neighbours, CB * cb);
		template<typename CB>
		static void filter(Minefield * f, int * neighbours, CB * cb);
		void filterunknown(int *);
		int countunknown(int *);
		int countflags(int *);
		void neighbourdifference(int *, int *);

		bool issubset(int *superset, int *subset, int length);
		Action **act_singlecheck(int idx);
		Action **act_dualcheck(int idx);
};

#endif
