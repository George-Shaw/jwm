/**
 * @file place.h
 * @author Joe Wingbermuehle
 * @date 2004-2006
 *
 * @brief Header for client placement functions.
 *
 */

#ifndef PLACE_H
#define PLACE_H

struct ClientNode;
struct ScreenType;
struct TrayType;

/** Bounding box. */
typedef struct BoundingBox {
   int x;         /**< x-coordinate of the bounding box. */
   int y;         /**< y-coordinate of the bounding box. */
   int width;     /**< Width of the bounding box. */
   int height;    /**< Height of the bounding box. */
} BoundingBox;

/*@{*/
#define InitializePlacement() (void)(0)
void StartupPlacement(void);
void ShutdownPlacement(void);
#define DestroyPlacement()    (void)(0)
/*@}*/

/** Remove struts associated with a client.
 * @param np The client.
 */
void RemoveClientStrut(struct ClientNode *np);

/** Read struts associated with a client.
 * @param np The client.
 */
void ReadClientStrut(struct ClientNode *np);

/** Place a client on the screen.
 * @param np The client to place.
 * @param alreadyMapped 1 if already mapped, 0 if unmapped.
 */
void PlaceClient(struct ClientNode *np, char alreadyMapped);

/** Place a maximized client on the screen.
 * @param np The client to place.
 * @param horiz Set if maximizing horizontally.
 * @param vert Set if maximizing vertically.
 */
void PlaceMaximizedClient(struct ClientNode *np, char horiz, char vert);

/** Move a client window for a border.
 * @param np The client.
 * @param negate 0 to gravitate for a border, 1 to gravitate for no border.
 */
void GravitateClient(struct ClientNode *np, char negate);

/** Get the x and y deltas for gravitating a client.
 * @param np The client.
 * @param gravity The gravity to use.
 * @param x Location to store the x delta.
 * @param y Location to store the y delta.
 */
void GetGravityDelta(const struct ClientNode *np, int gravity, int *x, int *y);

/** Constrain the size of a client.
 * @param np The client.
 * @return 1 if the size changed, 0 otherwise.
 */
char ConstrainSize(struct ClientNode *np);

/** Constrain the position of a client.
 * @param np The  client.
 */
void ConstrainPosition(struct ClientNode *np);

/** Get the bounding box for the screen.
 * @param sp A pointer to the screen whose bounds to get.
 * @param box The bounding box for the screen.
 */
void GetScreenBounds(const struct ScreenType *sp, BoundingBox *box);

/** Subtract bounds for the configured trays.
 * @param tp The first tray to consider.
 * @param box The bounding box.
 * @param layer The maximum layer of the tray bounds.
 */
void SubtractTrayBounds(const struct TrayType *tp, BoundingBox *box,
                        unsigned int layer);

#endif /* PLACE_H */

