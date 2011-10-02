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

		void filterunknown(int *) const;
		int countunknown(const int *) const;
		int countflags(const int *) const;
		void neighbourdifference(int * A, const int * B); // Removes all of B from A

		static bool issubset(const int *superset, const int *subset, int length);
		Action **act_singlecheck(int idx);
		Action **act_dualcheck(int idx);
};

#endif
