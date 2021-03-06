/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 2017 Free Software Foundation.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================

Author:  Gaius Mulley  <gaius@gnu.org>
*/

#define PYBOT_C
#include "pybot.h"

#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <unistd.h>
#  include <limits.h>

#include "sys/platform.h"
#include "idlib/LangDict.h"
#include "idlib/Timer.h"
#include "idlib/Str.h"

#include "framework/async/NetworkSystem.h"
#include "framework/BuildVersion.h"
#include "framework/DeclEntityDef.h"
#include "framework/FileSystem.h"
#include "renderer/ModelManager.h"

#include "gamesys/SysCvar.h"
#include "gamesys/SysCmds.h"
#include "script/Script_Thread.h"

#include "ai/AI.h"
#include "anim/Anim_Testmodel.h"
#include "Camera.h"
#include "SmokeParticles.h"
#include "Player.h"
#include "WorldSpawn.h"
#include "Misc.h"
#include "Trigger.h"
#include "Game_local.h"

const bool debugging = true;
const bool protocol_debugging = true;

#define S(x) #x
#define S_(x) S(x)
#define S__LINE__ S_(__LINE__)
# define pybot_debugf(X)  do { debugf (__FILE__ ":" S__LINE__ ":" ":" X) ; } while (0)

int define (const char *name, idAI *idBot);
int checkId (int id);
void initialise_dictionary (void);


void debugf (const char *s)
{
  if (debugging)
    gameLocal.Printf (s);
}

static void localExit (int code)
{
  gameLocal.Printf (__FILE__ ": exiting with code %d\n", code);
  exit (code);
}

#define MAX_PY_SERVERS 1000
#define MAX_PYLIST MAX_PY_SERVERS
#define ERROR(X)  do { pybot_debugf ("internal error: " X "\n"); localExit (1); } while (0)

static int superServerPort = 7000;

static pyServerClass *super = NULL;


char *getHome (void)
{
  return getenv ("HOME");
}


const char *getDir (void)
{
  return ".local/share/dhewm3/base/maps";
}


#define MAX_NAME 100


class item
{
 public:
  item (void);
  ~item (void);
  item (const item &from);
  item (const char *n, idAI *idBot);
  item (const char *n, idPlayer *ip);
  idVec3 getPos (void);
  bool stepDirection (float dir);
  int stepForward (int vel, int dist);
  int stepRight (int vel, int dist);
  int stepVec (int velforward, int velright, int dist);
  int start_firing (void);
  int stop_firing (void);
  int ammo (void);
  int weapon (int new_weapon);
  int getcurrentweapon (void);
  int health (void);
  int armor (void);
  int angle (void);
  void reload_weapon (void);
  bool aim (idEntity *enemy);
  int turn (int angle, int angle_vel);
  idEntity *getIdEntity (void);
  void select (int mask);

 private:
  enum {item_none, item_monster, item_player, item_ammo} kind;
  idAI *idai;
  idPlayer *idplayer;
  char name[MAX_NAME];
};


item::item (void)
  : kind (item_none), idai (NULL)
{
}


item::~item (void)
{
}


item::item (const item &from)
{
  *this = from;
}


item::item (const char *n, idAI *idBot)
{
  kind = item_monster;
  strcpy (name, n);
  idai = idBot;
}


item::item (const char *n, idPlayer *ip)
{
  kind = item_player;
  strcpy (name, n);
  idplayer = ip;
}


/*
 *  getPos - return the position of the item.
 */

idVec3 item::getPos (void)
{
  switch (kind)
    {
    case item_monster:
      return idai->GetPos ();
    case item_player:
      return idplayer->GetPos ();
    }
  ERROR ("case needs finishing");
  return idVec3 {0.0, 0.0, 0.0};
}


/*
 *  stepDirection - step forward in direction, dir.
 */

bool item::stepDirection (float dir)
{
  switch (kind)
    {
    case item_monster:
      return idai->StepDirection (dir);
#if 0
    case item_player:
      return idplayer->StepDirection (dir);
#endif
    }
  return false;
}


/*
 *  stepRight - step forward, units.
 */

int item::stepRight (int vel, int dist)
{
  switch (kind)
    {
#if 0
    case item_monster:
      return idai->StepDirection (vel, dist);
#endif
    case item_player:
      return idplayer->setRight (vel, dist);
    }
  return 0;
}


/*
 *  stepForward - step forward at velocity, vel, and over distance, dist.
 */

int item::stepForward (int vel, int dist)
{
  switch (kind)
    {
#if 0
    case item_monster:
      return idai->StepDirection (vel, dist);
#endif
    case item_player:
      return idplayer->setForward (vel, dist);
    }
  return 0;
}


/*
 *  stepForward - step forward at velocity, vel, and over distance, dist.
 */

int item::stepVec (int velforward, int velright, int dist)
{
  switch (kind)
    {
#if 0
    case item_monster:
      return idai->StepDirection (vel, dist);
#endif
    case item_player:
      return idplayer->setVec (velforward, velright, dist);
    }
  return 0;
}


/*
 *  aim - aim our weapon at enemy.
 *        True is returned if the enemy is visible.
 *        False is returned if the enemy is not visible.
 */

bool item::aim (idEntity *enemy)
{
  switch (kind)
    {
#if 0
    case item_monster:
      return idai->Aim (enemy);   // --fixme-- is this correct?
#endif
    case item_player:
      return idplayer->Aim (enemy);
    }
 return false;
}


/*
 *  turn - turn bot to angle at speed, angle_vel.
 *         The current angle of the bot is returned.
 */

int item::turn (int angle, int angle_vel)
{
  switch (kind)
    {
#if 0
    case item_monster:
      return idai->Turn (angle, angle_vel);
#endif
    case item_player:
      return idplayer->Turn (angle, angle_vel);
    }
  assert (false);
  return 0;
}


/*
 *  weapon - attempt to select weapon, new_weapon.
 *           If successful return the amount of ammo else return -1.
 */

int item::weapon (int new_weapon)
{
  switch (kind)
    {
    case item_player:
      return idplayer->ChangeWeapon (new_weapon);
    }
  assert (false);
  return 0;
}

/*
 *  getcurrentweapon - gets the current weapon of the player
 */

int item::getcurrentweapon ()
{
  switch (kind)
    {
    case item_monster:
      assert (false);
      return 0;  // ignore
    case item_player:
      return idplayer->getWeapon();
    }
}


/*
 *  start_firing - start firing and return the amount of ammo.
 */

int item::start_firing (void)
{
  switch (kind)
    {
#if 0
    case item_monster:
      return idai->StartFire ();  // --fixme-- is this correct?
#endif
    case item_player:
      return idplayer->Fire (true);
    }
  assert (false);
  return 0;
}


/*
 *  stop_firing - stop firing and return the amount of ammo.
 */

int item::stop_firing (void)
{
  switch (kind)
    {
#if 0
    case item_monster:
      return idai->StopFire ();  // --fixme-- is this correct?
#endif
    case item_player:
      return idplayer->Fire (false);
    }
  assert (false);
  return 0;
}


/*
 */

int item::ammo (void)
{
  return idplayer->weapon.GetEntity()->AmmoInClip();
}


/*
 *  health - returns the health of the bot
 */

int item::health (void)
{
  switch (kind)
    {
    case item_monster:
      assert (false);
      return 0;  // ignore
      break;
    case item_player:
      return idplayer->health;
    }
  assert (false);
  return 0;
}

/*
 *  armor - returns the armor of the bot
 */


int item::armor (void)
{

  switch(kind)
    {
    case item_monster:
      assert (false);
      return 0; //ignore
      break;
    case item_player:
      return idplayer->getArmor();
    }
  assert (false);
  return 0;
}

/*
 *  angle -
 */

int item::angle (void)
{
  switch (kind)
    {
    case item_monster:
      assert (false);
      return 0;  // ignore
      break;
    case item_player:
      return idplayer->GetYaw ();
    }
  assert (false);
  return 0;
}


/*
 *  reload_weapon
 */

void item::reload_weapon (void)
{

}


/*
 *  select
 */

void item::select (int mask)
{
  switch (kind)
    {
#if 0
    case item_monster:
      idai->select (mask);  // --fixme-- is this correct?
#endif
    case item_player:
      idplayer->select (mask);
    }
}


idEntity *item::getIdEntity (void)
{
  switch (kind)
    {
    case item_monster:
      return idai;
    case item_player:
      return idplayer;
    }
  assert (false);
  return NULL;
}


#define MAX_ENTRY 1000

class dict
{
 public:
  dict (void);
  ~dict (void);
  dict (const dict &from);

  int add (const char *name, idAI *idBot);
  int add (const char *name, idPlayer *ip);
  int checkId (int id);
  idVec3 getPos (int id);
  bool stepDirection (int id, float dir);
  int stepForward (int id, int vel, int units);
  int stepRight (int id, int vel, int dist);
  int stepVec (int id, int velforward, int velright, int dist);
  int start_firing (int id);
  int stop_firing (int id);
  int reload_weapon (int id);
  int ammo (int id);
  int health (int id);
  int armor (int id);
  int angle (int id);
  bool aim (int id, int enemy);
  int turn (int id, int angle, int angle_vel);
  void select (int id, int mask);
  int getHigh (void);
  int weapon (int id, int new_weapon);
  int getcurrentweapon (int id);
 private:
  item *entry[MAX_ENTRY];
  int high;
};


dict::dict (void)
  : high (1)
{
}


dict::~dict (void)
{
}


dict::dict (const dict &from)
{
  *this = from;
}


/*
 *  add - add a new monster entry into our dictionary of items.
 *        Return the id.
 */

int dict::add (const char *name, idAI *idBot)
{
  if (high == MAX_ENTRY)
    ERROR ("increase MAX_ENTRY");
  /*
   *  id of zero is not used.
   */
  int lastused = high;
  entry[lastused] = new item (name, idBot);
  high++;
  return lastused;
}


/*
 *  add - add a new monster entry into our dictionary of items.
 *        Return the id.
 */

int dict::add (const char *name, idPlayer *ip)
{
  if (high == MAX_ENTRY)
    ERROR ("increase MAX_ENTRY");
  /*
   *  id of zero is not used.
   */
  int lastused = high;
  entry[lastused] = new item (name, ip);
  high++;
  return lastused;
}


int dict::checkId (int id)
{
  if (id < MAX_ENTRY)
    return id;
  return 0;
}


idVec3 dict::getPos (int id)
{
  return entry[id]->getPos ();
}


/*
 *  stepDirection - step forward
 */

bool dict::stepDirection (int id, float dir)
{
  return entry[id]->stepDirection (dir);
}


/*
 *  stepRight - step right by, units.
 */

int dict::stepRight (int id, int vel, int dist)
{
  return entry[id]->stepRight (vel, dist);
}


/*
 *  stepForward - step right by, units.
 */

int dict::stepForward (int id, int vel, int dist)
{
  return entry[id]->stepForward (vel, dist);
}


/*
 *  stepVec - step forward and right simulataneously by dist units.
 */

int dict::stepVec (int id, int velforward, int velright, int dist)
{
  return entry[id]->stepVec (velforward, velright, dist);
}


/*
 *  start_firing - fire the weapon.
 */

int dict::start_firing (int id)
{
  return entry[id]->start_firing ();
}


/*
 *  stop_firing - stop firing the weapon.
 */

int dict::stop_firing (int id)
{
  return entry[id]->stop_firing ();
}


/*
 *  ammo - return the ammo available for the current weapon.
 */

int dict::ammo (int id)
{
  return entry[id]->ammo ();
}


/*
 *  health - return the health for the bot.
 */

int dict::health (int id)
{
  return entry[id]->health ();
}

/*
 * armor - return the armor for the bot.
 */

int dict::armor (int id)
{
  return entry[id]->armor ();
}

/*
 *  angle - return the angle (yaw) for the bot.
 */

int dict::angle (int id)
{
  return entry[id]->angle ();
}


/*
 *  aim - try and aim at enemy.
 *        If enemy is line of sight visible return true
 *        else return false.
 */

bool dict::aim (int id, int enemy)
{
  return entry[id]->aim (entry[enemy]->getIdEntity ());
}


/*
 *  turn - turn bot to face, angle.  It returns the current angle
 *         of the bot.
 */

int dict::turn (int id, int angle, int angle_vel)
{
  return entry[id]->turn (angle, angle_vel);
}


/*
 *  weapon - change to new_weapon and return the amount of
 *           ammo for this weapon.  -1 if the weapon is not
 *           in the inventory.
 */

int dict::weapon (int id, int new_weapon)
{
  return entry[id]->weapon (new_weapon);
}

/*
 *  weapon - change to new_weapon and return the amount of
 *           ammo for this weapon.  -1 if the weapon is not
 *           in the inventory.
 */

int dict::getcurrentweapon (int id)
{
  return entry[id]->getcurrentweapon ();
}

/*
 *  select - wait for bot id to complete any activity defined in mask.
 */

void dict::select (int id, int mask)
{
  entry[id]->select (mask);
}


/*
 *  getHigh - return the last legal id in the dictionary.
 */

int dict::getHigh (void)
{
  return high-1;
}


static dict *dictionary = NULL;


void initialise_dictionary (void)
{
  if (dictionary == NULL)
    dictionary = new dict ();
}


int checkId (int id)
{
  initialise_dictionary ();
  return dictionary->checkId (id);
}


int define (const char *name, idAI *idBot)
{
  initialise_dictionary ();
  return dictionary->add (name, idBot);
}


int define (const char *name, idPlayer *ip)
{
  initialise_dictionary ();
  return dictionary->add (name, ip);
}


class pyList
{
public:
  pyList (void);
  ~pyList (void);
  pyList (const pyList &from);

  void include (pyBotClass *p);
  pyBotClass *remove (const char *name);
private:
  pyBotClass *content[MAX_PYLIST];
  int high;
  int used;
};

pyList::pyList (void)
  : high (0), used (0)
{
}

pyList::~pyList (void)
{
}

pyList::pyList (const pyList &from)
{
  *this = from;
}


void pyList::include (pyBotClass *p)
{
  /*
   *  have we already stored p?  if so we return.
   */
  for (int i = 0; i < high; i++)
    {
      if (content[i] == p)
	return;   /* already stored, we are done.  */
    }
  if (used == MAX_PYLIST)
    ERROR ("increase MAX_PYLIST");
  /*
   *  check to reuse an empty slot.
   */
  for (int i = 0; i < high; i++)
    {
      if (content[i] == NULL)
	{
	  content[i] = p;
	  used++;
	  return;
	}
    }
  /*
   *  we have to use a new slot.
   */
  content[high] = p;
  high++;
  used++;
}


/*
 *  remove - attempt to remove bot with a name from the pyList.
 *           NULL is returned if unsuccessful.
 */

pyBotClass *pyList::remove (const char *name)
{
  for (int i = 0; i < high; i++)
    {
      gameLocal.Printf ("buffer [%d] = %s    looking for %s\n", i, content[i]->getName (), name);
      if ((content[i] != NULL) && (strcmp (name, content[i]->getName ()) == 0))
	{
	  pyBotClass *p = content[i];
	  content[i] = NULL;
	  used--;
	  gameLocal.Printf ("buffer [%d] = %s found\n", i, p->getName ());
	  return p;
	}
    }
  return NULL;
}


/***********************************************************************/
/* end of test */
/***********************************************************************/

/*
 *  list of pending bots which have been partially connected.
 *  This is necessary as the clients might try and connect in any order.
 *  Whereas the map creation will want a specific bot.
 */

static pyList pending;

/*
 *  active list contains all pybots which have been attached
 *  to their python clients.
 */
static pyList active;


pyBufferClass::pyBufferClass (void)
  : rin (0), win (0), wout (0), wsize (0)
{
}


pyBufferClass::~pyBufferClass (void)
{
}


pyBufferClass::pyBufferClass (const pyBufferClass &from)
{
  *this = from;
}


char *pyBufferClass::pyread (int fd, bool canBlock)
{
  fd_set inset;
  int n;

  while (true)
    {
      FD_ZERO (&inset);
      FD_SET (fd, &inset);
      if (canBlock)
	select (fd+1, &inset, NULL, NULL, NULL);
      else
	{
	  struct timeval timeout;
	  timeout.tv_sec = 0;
	  timeout.tv_usec = 0;
	  select (fd+1, &inset, NULL, NULL, &timeout);
	}

      if (FD_ISSET (fd, &inset)) {
	n = read (fd, &rBuffer[rin], MAX_PY_BUFFER-rin);
	if (n > 0)
	  {
	    int t = rin + n;
	    for (int i = rin; i < t; i++)
	      if (rBuffer[i] == '\n')
		{
		  /*
		   *  end of line.  Return this and reset input.
		   */
		  rBuffer[i] = (char)0;  /* convert the '\n' into '\0'.  */
		  rin += n;
		  rBuffer[rin] = (char)0;  /* null terminate the buffer for debugging purposes.  */
		  rin = 0;
                  if (protocol_debugging)
                    {
                      gameLocal.Printf ("rBuffer is now: ");
                      gameLocal.Printf (rBuffer);
                    }
		  return &rBuffer[0];
		}
	    /*
	     *  partial line read, continue reading later.
	     */
	    rin += n;
	  }
	else
	  return NULL;
      } else
	return NULL;
    }
  return NULL;
}


void pyBufferClass::pyput (char *output)
{
  for (int i = 0; i < strlen (output); i++)
    pyputChar (output[i]);
}


void pyBufferClass::pyputChar (char ch)
{
  if (wsize == MAX_PY_BUFFER)
    ERROR ("increase MAX_PY_BUFFER");
  wBuffer[win] = ch;
  win = (win + 1) % MAX_PY_BUFFER;
  wsize++;
}


bool pyBufferClass::pyflushed (int fd, bool canBlock)
{
  if (wsize == 0)
    return true;

  fd_set outset;
  int n;

  while (wout < wsize) {
    FD_ZERO (&outset);
    FD_SET (fd, &outset);
    if (canBlock)
      select (fd+1, NULL, &outset, NULL, NULL);
    else {
      struct timeval timeout;
      timeout.tv_sec = 0;
      timeout.tv_usec = 0;
      select (fd+1, NULL, &outset, NULL, &timeout);
    }

    if (FD_ISSET (fd, &outset)) {
      if (protocol_debugging)
	{
	  for (int i = wout; i < wsize-wout; i++)
	    {
	      gameLocal.Printf ("buffer sending %c\n", wBuffer[i]);
	    }
	}
      n = write (fd, &wBuffer[wout], wsize-wout);
      if (n > 0)
	wout += n;
      else
	return false;
    } else
      return false;
  }
  wout = 0;
  wsize = 0;
  win = 0;
  return true;
}

/* ok */

pyServerClass::pyServerClass (void)
  : enabled (false), state (toInit), connectedBot (NULL)
{
}


pyServerClass::~pyServerClass (void)
{
}


pyServerClass::pyServerClass (const pyServerClass &from)
{
  *this = from;
}


void checkInitialiseSuper (void)
{
  if (super == NULL)
    super = new pyServerClass ();
}


/*
 *  poll the super server.
 */

void poll (bool canBlock)
{
  checkInitialiseSuper ();
  super->poll (canBlock);
}


void pyServerClass::initServer (void)
{
  superServerPort = tryActivate (superServerPort);
}


void pyServerClass::poll (bool canBlock)
{
  switch (state) {

  case toInit:
    initServer ();
    break;
  case toAccept:
    acceptServer (canBlock);
    break;
  case toRead:
    readServer (canBlock);
    break;
  case toWrite:
    writeServer (canBlock);
    break;
  case toWriteConnected:
    writeConnectedServer (canBlock);
    break;
  case toClose:
    closeServer (canBlock);
    break;
  default:
    ERROR ("unrecognised state");
  }
}


/*
 *  tryActivate - try and use desiredPort when starting a socket server.
 */

int pyServerClass::tryActivate (int desiredPort)
{
  struct hostent *hp;
  char hostname[HOST_NAME_MAX];

  if (gethostname (hostname, HOST_NAME_MAX) < 0)
    ERROR ("cannot find our hostname (is networking operational on this machine?)");

  hp = gethostbyname (hostname);

  int n = 0;
  int b;
  do {
    /*
     *  open a TCP socket (an Internet stream socket)
     */

    socketFd = socket (hp->h_addrtype, SOCK_STREAM, 0);
    if (socketFd < 0)
      ERROR ("socket");

    socklen_t s = sizeof (sa);

    memset (&sa, 0, s);
    assert ((hp->h_addrtype == AF_INET));
    sa.sin_family      = hp->h_addrtype;
    sa.sin_addr.s_addr = htonl (INADDR_ANY);
    sa.sin_port        = htons (desiredPort);

    b = bind (socketFd, (struct sockaddr *)&sa, s);

    if (b < 0) {
      gameLocal.Printf ("unable to bind python socket to the desired port %d\n",
			desiredPort);
      n++;
      desiredPort++;
    }
  } while ((b < 0) && (n < MAX_PY_SERVERS));

  if (b < 0)
    ERROR ("cannot bind to any socket on this machine");

  gameLocal.Printf ("waiting for python bot to connect on port %d\n", desiredPort);
  listen (socketFd, 1);
  state = toAccept;
  return desiredPort;
}


void pyServerClass::acceptServer (bool canBlock)
{
  socklen_t n = sizeof (isa);
  connectFd = accept (socketFd, (struct sockaddr *)&isa, &n);
  if (connectFd < 0)
    ERROR ("accept");

  state = toRead;
}


void pyServerClass::readServer (bool canBlock)
{
  char *data = buffer.pyread (connectFd, canBlock);  /* data contains the bot name or "super"  */

  if (data != NULL)
    {
      char portValue[20];
      if (strcmp (data, "super") == 0)
	{
	  gameLocal.Printf ("bot wants to know the superserver port (which is %d)\n",
			    superServerPort);
	  /*
	   *  found query for superServer port
	   */
	  idStr::snPrintf (portValue, sizeof (portValue), "%d\n", superServerPort);
	  buffer.pyput (portValue);
	  state = toWrite;
	}
      else
	{
	  gameLocal.Printf ("bot %s wants to connect\n", data);
	  /*
	   *  python bot want us to create a bot and tell the script which portno
	   *  has been allocated.
	   */
	  int portno;

	  connectedBot = new pyBotClass ();
	  connectedBot->setName (strdup (data));
	  connectedBot->initServer (0);  /* any available port is good.  */
	  portno = connectedBot->getPortNo ();
	  gameLocal.Printf ("bot has been allocated port %d\n", portno);
	  idStr::snPrintf (portValue, sizeof (portValue), "%d\n", portno);
	  buffer.pyput (portValue);
	  state = toWriteConnected;
	}
    }
}


/*
 *  writeServer
 */

void pyServerClass::writeServer (bool canBlock)
{
  if (buffer.pyflushed (connectFd, canBlock))
    state = toClose;
}


/*
 *  writeConnectedServer
 */

void pyServerClass::writeConnectedServer (bool canBlock)
{
  if (buffer.pyflushed (connectFd, canBlock))
    {
      state = toClose;
      connectedBot->setConnected ();
      pending.include (connectedBot);
      connectedBot = NULL;
      gameLocal.Printf ("bot port has been flushed to the script\n");
    }
}


/*
 *  closeServer
 */

void pyServerClass::closeServer (bool canBlock)
{
  close (connectFd);
  state = toAccept;
}


/*
 *  developerHelp - writes out some interactive help to prompt the developer to manually connect
 *                  a Python bot.
 */

void developerHelp (const char *name)
{
  char buffer[HOST_NAME_MAX];
  int result = gethostname (buffer, sizeof (buffer));

  gameLocal.Printf ("bot %s is ready to be controlled by Python\n", name);
  gameLocal.Printf ("suggest that you run the following from the command line\npython %s/%s/%s.py\n", getHome (), getDir (), name);
  if (result < 0)
    gameLocal.Printf ("developer needs to run the Python script from the terminal connecting to localhost:%d", superServerPort);
  else
    gameLocal.Printf ("developer needs to run the Python script from the terminal connecting to %s:%d",
	    buffer, superServerPort);
}


/*
 *  forkscript - if in developer mode call developerHelp otherwise fork and exec a python script.
 */

void forkScript (const char *name)
{
  if (getenv ("DEBUG_PYBOT") == NULL)
    {
      char buffer[PATH_MAX];

      idStr::snPrintf (buffer, sizeof (buffer), "%s/%s/%s.py", getHome (), getDir (), name);
      gameLocal.Printf ("execl /usr/bin/python %s\n", buffer);
      int pid = fork ();
      if (pid == 0)
	/* child process.  */
	{
	  int r = execl ("/usr/bin/python", "python", buffer, (char *)NULL);

	  if (r != 0)
	    perror ("execl");
	}
      else
	/* add pid to list of pids and kill them off at the end of the game.  */
	;
    }
  else
    developerHelp (name);
}


static void mystop (void) {}


/*
 *  populateDictionary - adds idBot into the python bot universe of objects.
 */

void populateDictionary (const char *name, idAI *idBot)
{
  define (name, idBot);
}

/*
 *  populateDictionary - adds idBot into the python bot universe of objects.
 */

void populateDictionary (const char *name, idPlayer *ip)
{
  define (name, ip);
}


/*
 *  doRegisterName - registers the bot with the python client.
 *                   We create a server and tell the super server
 *                   which port we are using.
 *                   This bot (player/monster) is completely controlled via Python.
 */

pyBotClass *doRegisterName (const char *name, int id)
{
  pyBotClass *b;

  gameLocal.Printf ("real registerName reached!\n");
  forkScript (name);
  gameLocal.Printf ("registerName is wanting to connect with a script called '%s'\n", name);
  do {
    b = pending.remove (name);
    if (b == NULL)
      poll (true);
  } while (b == NULL);
  active.include (b);
  b->setId (id);
  gameLocal.Printf ("registerName completing after successfully connecting with the script '%s'\n", name);
  return b;
}


/*
 *  registerName - the map has requested a Python monster bot.
 *                 We create a server and tell the super server which port we are using.
 */

pyBotClass *registerName (const char *name, idAI *idBot)
{
  return doRegisterName (name, define (name, idBot));
}


/*
 *  registerName - the map has requested a Python player bot.
 *                 We create a server and tell the super server
 *                 which port we are using.
 *                 This bot is completely controlled via Python
 *                 and no user input is used.
 */

pyBotClass *registerName (const char *name, idPlayer *ip)
{
  return doRegisterName (name, define (name, ip));
}


pyBotClass::pyBotClass ()
  : myid (0), name (NULL), portNo (0), enabled (false), state (toInit),
    socketFd (0), connectFd (0), connected (false)
{
}


pyBotClass::~pyBotClass ()
{
}


pyBotClass::pyBotClass (const pyBotClass &from)
{
  *this = from;
}


int pyBotClass::getPortNo (void)
{
  return portNo;
}


bool pyBotClass::getConnected (void)
{
  checkInitialiseSuper ();
  super->poll (false);
  return connected;
}


void pyBotClass::setConnected (void)
{
  gameLocal.Printf ("setConnected has been called\n");
  mystop ();
  connected = true;
}


void pyBotClass::initServer (int desiredPort)
{
  int p;
  if (desiredPort > 0)
    {
      do {
	p = tryActivate (desiredPort);
	if (p == 0)
	  {
	    gameLocal.Printf ("waiting for port %d to become available...\n", desiredPort);
	    sleep (1);
	  }
      } while (p == 0);
    }
  else
    {
      desiredPort = superServerPort+1;
      do {
	p = tryActivate (desiredPort);
	desiredPort++;
      } while (p == 0);
    }
  portNo = p;
}


/*
 *  tryActivate - try and use desiredPort when starting a socket server.
 */

int pyBotClass::tryActivate (int desiredPort)
{
  struct hostent *hp;
  int b;
  char hostname[HOST_NAME_MAX];

  if (gethostname (hostname, HOST_NAME_MAX) < 0)
    ERROR ("cannot find our hostname (is networking operational on this machine?)");

  hp = gethostbyname (hostname);

  /*
   *  open a TCP socket (an Internet stream socket)
   */

  socketFd = socket (hp->h_addrtype, SOCK_STREAM, 0);
  if (socketFd < 0)
    ERROR ("socket");

  memset (&sa, 0, sizeof (sa));
  assert ((hp->h_addrtype == AF_INET));
  sa.sin_family      = hp->h_addrtype;
  sa.sin_addr.s_addr = htonl (INADDR_ANY);
  sa.sin_port        = htons (desiredPort);

  b = bind (socketFd, (struct sockaddr *)&sa, sizeof (sa));

  if (b < 0) {
    debugf ("unable to bind python socket to the desired port\n");
    return 0;
  }
  else
    {
      gameLocal.Printf ("waiting for python bot to connect on port %d\n", desiredPort);
      listen (socketFd, 1);
      state = toAccept;
    }
  return desiredPort;
}


/*
 *  acceptServer - bot performs an accept and obtains the connectFd socket for all
 *                 remaining communication with the python script.
 */

void pyBotClass::acceptServer (bool canBlock)
{
  socklen_t n = sizeof (isa);
  connectFd = accept (socketFd, (struct sockaddr *)&isa, &n);
  if (connectFd < 0)
    ERROR ("accept");

  state = toRead;
}


/*
 *  readServer - tries to obtain a command from the python script and if successful
 *               it interprets the remote procedure call.
 */

void pyBotClass::readServer (bool canBlock)
{
  char *data = buffer.pyread (connectFd, canBlock);  /* data contains the bot name or "super"  */

  if (data != NULL)
    interpretRemoteProcedureCall (data);
}


/*
 *  setId - assign, myid, to, id.
 */

void pyBotClass::setId (int id)
{
  myid = id;
}


/*
 *  interpretRemoteProcedureCall - a switch statement of all rpc commands.
 */

void pyBotClass::interpretRemoteProcedureCall (char *data)
{
  if (protocol_debugging)
    gameLocal.Printf ("rpc (%s)\n", data);
  if (strcmp (data, "super") == 0)
    rpcSuper ();
  else if (idStr::Cmpn (data, "getpos ", 7) == 0)   // Id's strncmp equivalent
    rpcGetPos (&data[7]);
  else if (strcmp (data, "self") == 0)
    rpcSelf ();
  else if (strcmp (data, "health") == 0)
    rpcHealth ();
  else if (strcmp (data, "armor") == 0)
    rpcArmor ();
  else if (strcmp (data, "maxobj") == 0)
    rpcMaxObj ();
  else if (idStr::Cmpn (data, "step ", 5) == 0)
    rpcStep (&data[5]);
  else if (idStr::Cmpn (data, "right ", 6) == 0)
    rpcRight (&data[6]);
  else if (idStr::Cmpn (data, "forward ", 8) == 0)
    rpcForward (&data[8]);
  else if (idStr::Cmpn (data, "stepvec ", 8) == 0)
    rpcStepVec (&data[8]);
  else if (strcmp (data, "start_firing") == 0)
    rpcStartFiring ();
  else if (strcmp (data, "stop_firing") == 0)
    rpcStopFiring ();
  else if (strcmp (data, "reload_weapon") == 0)
    rpcReloadWeapon ();
  else if (strcmp (data, "ammo") == 0)
    rpcAmmo ();
  else if (idStr::Cmpn (data, "aim ", 4) == 0)
    rpcAim (&data[4]);
  else if (idStr::Cmpn (data, "turn ", 5) == 0)
    rpcTurn (&data[5]);
  else if (strcmp (data, "angle") == 0)
    rpcAngle ();
  else if (strcmp (data, "penmap") == 0)
    rpcPenMap ();
  else if (idStr::Cmpn (data, "select ", 7) == 0)
    rpcSelect (&data[7]);
  else if (idStr::Cmpn (data, "get_class_name_entity ", 22) == 0)
    rpcGetClassNameEntity (&data[22]);
  else if (idStr::Cmpn (data, "get_pair_name_entity ", 21) == 0)
    rpcGetPairEntity (&data[21]);
  else if (idStr::Cmpn (data, "get_entity_pos ", 15) == 0)
    rpcGetEntityPos (&data[15]);
  else if (idStr::Cmpn (data, "change_weapon ", 14) == 0)
    rpcChangeWeapon (&data[14]);
  else if (idStr::Cmpn (data, "get_current_weapon ", 18) == 0)
    rpcGetCurrentWeapon (&data[18]);
  else
    {
      gameLocal.Printf ("data = \"%s\", len (data) = %d\n", data, (int) strlen (data));
      ERROR ("unrecognised rpc command");
    }
}


/*
 *  rpcSuper - handle the request for the superserver portno.
 */

void pyBotClass::rpcSuper (void)
{
  char portValue[20];

  gameLocal.Printf ("bot wants to know the superserver port (which is %d) interesting as this has been detected by a botserver\n", superServerPort);
  /*
   *  found query for superServer port, respond appropriately
   */
  idStr::snPrintf (portValue, sizeof (portValue), "%d\n", superServerPort);
  buffer.pyput (portValue);
  state = toWrite;
}


/*
 *  rpcGetPos - generate an x, y, z string and send it back to the script.
 *              The parameter, data, contains the id.
 */

void pyBotClass::rpcGetPos (char *data)
{
  if (protocol_debugging)
    gameLocal.Printf ("rpcGetPos (%s) call by python\n", data);

  char buf[1024];
  int id = checkId (atoi (data));

  if (id > 0)
    {
      const idVec3 &org = dictionary->getPos (id);
      idStr::snPrintf (buf, sizeof (buf), "%g %g %g\n",
		       org.x, org.y, org.z);
    }
  else
    strcpy (buf, "error invalid id sent to getpos\n");
  if (protocol_debugging)
    gameLocal.Printf ("rpcGetPos responding with: %s\n", buf);
  buffer.pyput (buf);
  state = toWrite;
}


/*
 *  rpcSelf - return our id.
 */

void pyBotClass::rpcSelf (void)
{
  char buf[1024];

  if (protocol_debugging)
    gameLocal.Printf ("rpcSelf called by python\n");
  idStr::snPrintf (buf, sizeof (buf), "%d\n", myid);
  if (protocol_debugging)
    gameLocal.Printf ("rpcSelf responding with: %s\n", buf);
  buffer.pyput (buf);
  state = toWrite;
}


/*
 *  rpcHealth - return our health.
 */

void pyBotClass::rpcHealth (void)
{
  char buf[1024];

  if (protocol_debugging)
    gameLocal.Printf ("rpcHealth called by python\n");
  idStr::snPrintf (buf, sizeof (buf), "%d\n", dictionary->health (myid));
  if (protocol_debugging)
    gameLocal.Printf ("rpcHealth responding with: %s\n", buf);
  buffer.pyput (buf);
  state = toWrite;
}

/*
 *  rpcArmor - return our bots armor
 */
void pyBotClass::rpcArmor (void)
{
  char buf[1024];

  if (protocol_debugging)
    gameLocal.Printf ("rpcArmor called by python\n");
  idStr::snPrintf (buf, sizeof (buf), "%d\n", dictionary->armor (myid));
  if (protocol_debugging)
    gameLocal.Printf ("rpcArmor responding with: %s\n", buf);
  buffer.pyput (buf);
  state = toWrite;
}

/*
 *  rpcMaxObj - return the maximum number of ids in the dictionary.
 *              Indicates that 1..high can all be accessed.
 */

void pyBotClass::rpcMaxObj (void)
{
 char buf[1024];

 initialise_dictionary ();
 idStr::snPrintf (buf, sizeof (buf), "%d\n", dictionary->getHigh ());
 buffer.pyput (buf);
 state = toWrite;
}


/*
 *  rpcStep - step forward direction
 *            The parameter, data, contains the direction.
 */

void pyBotClass::rpcStep (char *data)
{
  char buf[1024];
  bool done = false;

  if (protocol_debugging)
    gameLocal.Printf ("rpcStep (%s) call by python\n", data);

  if (myid > 0)
    {
      float dir = atof (data);
      done = dictionary->stepDirection (myid, dir);
    }
  if (done)
    strcpy (buf, "true\n");
  else
    strcpy (buf, "false\n");
  buffer.pyput (buf);
  state = toWrite;
}


/*
 *  rpcRight - step right.
 *             The parameter, data, contains two parameters:  velocity and distance.
 */

void pyBotClass::rpcRight (char *data)
{
  char buf[1024];
  int vel = 0;
  int dist = 0;

  if (protocol_debugging)
    gameLocal.Printf ("rpcRight (%s) call by python\n", data);

  if (myid > 0)
    {
      vel = atoi (data);
      char *p = index (data, ' ');
      if ((p == NULL) || ((*p) == '\0'))
	dist = 0;
      else
	dist = atoi (p);
      dist = dictionary->stepRight (myid, vel, dist);
    }
  idStr::snPrintf (buf, sizeof (buf), "%d\n", dist);
  buffer.pyput (buf);
  state = toWrite;
}


/*
 *  rpcForward - step forward.
 *               The parameter, data, contains two parameters:  velocity and distance.
 */

void pyBotClass::rpcForward (char *data)
{
  char buf[1024];
  int vel = 0;
  int dist = 0;

  if (protocol_debugging)
    gameLocal.Printf ("rpcForward (%s) call by python\n", data);

  if (myid > 0)
    {
      vel = atoi (data);
      char *p = index (data, ' ');
      if ((p == NULL) || ((*p) == '\0'))
	dist = 0;
      else
	dist = atoi (p);
      dist = dictionary->stepForward (myid, vel, dist);
    }
  idStr::snPrintf (buf, sizeof (buf), "%d\n", dist);
  buffer.pyput (buf);
  state = toWrite;
}


/*
 *  rpcStepVec - step forward along a vector.
 *               The parameter, data, contains three parameters:
 *               velocity forward, velocity right and distance.
 */

void pyBotClass::rpcStepVec (char *data)
{
  char buf[1024];
  int velforward = 0;
  int velright = 0;
  int dist = 0;

  if (protocol_debugging)
    gameLocal.Printf ("rpcStepVec (%s) call by python\n", data);

  if (myid > 0)
    {
      velforward = atoi (data);
      char *p = index (data, ' ');
      if ((p == NULL) || ((*p) == '\0'))
	velright = 0;
      else
	{
	  velright = atoi (p);
	  char *p = index (data, ' ');
	  if ((p == NULL) || ((*p) == '\0'))
	    dist = atoi (p);
	}
      dist = dictionary->stepVec (myid, velforward, velright, dist);
    }
  idStr::snPrintf (buf, sizeof (buf), "%d\n", dist);
  buffer.pyput (buf);
  state = toWrite;
}


/*
 *  rpcAim - aim the weapon at, id.
 */

void pyBotClass::rpcAim (char *data)
{
  char buf[1024];
  int id = checkId (atoi (data));
  bool seen = false;

  if (protocol_debugging)
    gameLocal.Printf ("rpcAim (%s) called by python\n", data);

  if (id > 0)
    seen = dictionary->aim (myid, id);
  if (seen)
    idStr::snPrintf (buf, sizeof (buf), "true\n");
  else
    idStr::snPrintf (buf, sizeof (buf), "false\n");
  buffer.pyput (buf);
  state = toWrite;
}


/*
 *  rpcAngle - return the angle of the bot.
 */

void pyBotClass::rpcAngle (void)
{
  char buf[1024];

  if (protocol_debugging)
    gameLocal.Printf ("rpcAngle called by python\n");

  int angle = dictionary->angle (myid);
  idStr::snPrintf (buf, sizeof (buf), "%d\n", angle);
  buffer.pyput (buf);
  state = toWrite;
}

/*
 *  rpcTurn - turn to face angle.
 */

void pyBotClass::rpcTurn (char *data)
{
  char buf[1024];
  int angle = 0;
  int vel = 0;

  if (protocol_debugging)
    gameLocal.Printf ("rpcAngle (%s) called by python\n", data);

  if (myid > 0)
    {
      angle = atoi (data);
      char *p = index (data, ' ');
      if ((p == NULL) || ((*p) == '\0'))
	vel = 0;
      else
	vel = atoi (p);
      angle = dictionary->turn (myid, angle, vel);
    }

  idStr::snPrintf (buf, sizeof (buf), "%d\n", angle);
  buffer.pyput (buf);
  state = toWrite;
}


/*
 *  rpcPenMap - request the pen mapname.
 */

void pyBotClass::rpcPenMap (void)
{
  char buf[1024];

  if (protocol_debugging)
    gameLocal.Printf ("rpcPenMap called by python\n");

  const char *p = gameLocal.FindDefinition ("penmap");
  if (p == NULL)
    strcpy (buf, "\n");
  else
    idStr::snPrintf (buf, sizeof (buf), "%s\n", p);
  buffer.pyput (buf);
  state = toWrite;
}


/*
 *  rpcSelect - request we wait until some desired activity completes.
 */

void pyBotClass::rpcSelect (char *data)
{
  if (protocol_debugging)
    gameLocal.Printf ("rpcSelect (%s) called by python\n", data);
  int mask = 0;

  if (strcmp (data, "any") == 0)
    mask = 0x0f;
  else
    mask = atoi (data);

  if (mask == 0 || myid == 0)
    {
      char buf[1024];

      idStr::snPrintf (buf, sizeof (buf), "0\n");
      buffer.pyput (buf);
      state = toWrite;
    }
  else
    {
      dictionary->select (myid, mask);
      state = toSelect;   /* move into the select state.  */
    }
}


/*
 *  rpcStartFiring - fire the weapon.
 */

void pyBotClass::rpcStartFiring (void)
{
  char buf[1024];
  int ammo;

  if (protocol_debugging)
    gameLocal.Printf ("rpcStartFiring call python\n");

  if (myid > 0)
    ammo = dictionary->start_firing (myid);
  else
    ammo = 0;

  idStr::snPrintf (buf, sizeof (buf), "%d\n", ammo);
  buffer.pyput (buf);
  state = toWrite;
}


/*
 *  rpcStopFiring - fire the weapon.
 */

void pyBotClass::rpcStopFiring (void)
{
  char buf[1024];
  int ammo;

  if (protocol_debugging)
    gameLocal.Printf ("rpcStopFiring call by python\n");

  if (myid > 0)
    ammo = dictionary->stop_firing (myid);
  else
    ammo = 0;

  idStr::snPrintf (buf, sizeof (buf), "%d\n", ammo);
  buffer.pyput (buf);
  state = toWrite;
}


/*
 *  rpcAmmo - return the amount of ammo available for the current weapon.
 */

void pyBotClass::rpcAmmo (void)
{
  char buf[1024];
  int ammo;

  if (protocol_debugging)
    gameLocal.Printf ("rpcAmmo call by python\n");

  if (myid > 0)
    ammo = dictionary->ammo (myid);
  else
    ammo = 0;

  idStr::snPrintf (buf, sizeof (buf), "%d\n", ammo);
  buffer.pyput (buf);
  state = toWrite;
}


/*
 *  rpcReloadWeapon - return the amount of ammo available for the current weapon
 *                    after reloading.
 */

void pyBotClass::rpcReloadWeapon (void)
{
  char buf[1024];
  int ammo;

  if (protocol_debugging)
    gameLocal.Printf ("rpcReloadWeapon call by python\n");

  if (myid > 0)
    ammo = dictionary->ammo (myid);   // --fixme-- this should call something else
  else
    ammo = 0;

  idStr::snPrintf (buf, sizeof (buf), "%d\n", ammo);
  buffer.pyput (buf);
  state = toWrite;
}


/*
 *  rpcGetClassNameEntity - lookup the number of the entity containing classname, data.
 *                          -1 is returned on failure.
 */

void pyBotClass::rpcGetClassNameEntity (char *data)
{
  char buf[1024];

  if (protocol_debugging)
    gameLocal.Printf ("rpcGetClassNameEntity (%s) called by python\n", data);

  int ent_no = gameLocal.FindEntityFromName (data);

  idStr::snPrintf (buf, sizeof (buf), "%d\n", ent_no);
  buffer.pyput (buf);
  state = toWrite;
}


/*
 *  rpcGetPairEntity - lookup the number of the entity containing the strings:  a, b.
 *                     arg contains string, a, followed by string, b.
 *                     -1 is returned on failure.
 */

void pyBotClass::rpcGetPairEntity (char *arg)
{
  char buf[1024];
  char *a = arg;

  if (protocol_debugging)
    gameLocal.Printf ("rpcGetPairEntity (%s) called by python\n", arg);

  char *b = index (arg, ' ');
  if (b != NULL)
    {
      *b = (char)0;
      b++;
    }

  int ent_no = gameLocal.FindEntityFromPair (a, b);

  idStr::snPrintf (buf, sizeof (buf), "%d\n", ent_no);
  buffer.pyput (buf);
  state = toWrite;
}


/*
 *  rpcGetEntityPos - generate an x, y, z string and send it back to the script.
 *                    The parameter, data, contains the entity number.
 */

void pyBotClass::rpcGetEntityPos (char *data)
{
  if (protocol_debugging)
    gameLocal.Printf ("rpcGetentityPos (%s) call by python\n", data);

  char buf[1024];
  int id = checkId (atoi (data));

  if (id >= 0)
    {
      const idVec3 &org = gameLocal.GetEntityOrigin (id);

      idStr::snPrintf (buf, sizeof (buf), "%g %g %g\n",
		       org.x, org.y, org.z);
    }
  else
    strcpy (buf, "error invalid id sent to getentitypos\n");
  if (protocol_debugging)
    gameLocal.Printf ("rpcGetEntityPos responding with: %s\n", buf);
  buffer.pyput (buf);
  state = toWrite;
}


/*
 *  rpcChangeWeapon - attempt to change weapon to the number in data.
 *                    The amount of ammo is returned.  -1 means no weapon.
 */

void pyBotClass::rpcChangeWeapon (char *data)
{
  if (protocol_debugging)
    gameLocal.Printf ("rpcChangeWeapon (%s) call by python\n", data);

  char buf[1024];
  int weapon = atoi (data);
  int ammo = -1;

  if (weapon >= 0)
    ammo = dictionary->weapon (myid, weapon);
  idStr::snPrintf (buf, sizeof (buf), "%d\n", ammo);
  if (protocol_debugging)
    gameLocal.Printf ("rpcChangeWeapon responding with: %s\n", buf);
  buffer.pyput (buf);
  state = toWrite;
}

/*
 *  rpcGetCurrentWeapon - gets the weapon the bot is current holding
 */

void pyBotClass::rpcGetCurrentWeapon (char *data)
{
  char buf[1024];

  if (protocol_debugging)
    gameLocal.Printf ("rpcGetCurrentWeapon (%s) call by python\n", data);
  idStr::snPrintf (buf, sizeof (buf), "%d\n", dictionary->getcurrentweapon (myid));
  if (protocol_debugging)
    gameLocal.Printf ("rpcGetCurrentWeapon responding with: %s\n", buf);
  buffer.pyput (buf);
  state = toWrite;
}

/*
 *  writeServer
 */

void pyBotClass::writeServer (bool canBlock)
{
  if (buffer.pyflushed (connectFd, canBlock))
    state = toRead;
}


/*
 *  closeServer
 */

void pyBotClass::closeServer (bool canBlock)
{
  close (connectFd);
  state = toAccept;
}


/*
 *  selectServer
 */

void pyBotClass::selectServer (bool canBlock)
{
  /* currently does nothing.  As we are still waiting for the bot to
     complete some activity.  */
}


/*
 *  selectComplete - if the bot had called select then it will be
 *                   waiting for a reply.  We send true and move to
 *                   the toWrite state.
 */

void pyBotClass::selectComplete (int mask)
{
  if (state == toSelect)
    {
      char buf[1024];

      idStr::snPrintf (buf, sizeof (buf), "%d\n", mask);
      buffer.pyput (buf);
      state = toWrite;
      if (protocol_debugging)
	gameLocal.Printf ("selectComplete (%d)\n", mask);
    }
}


void pyBotClass::poll (bool canBlock)
{
  switch (state) {

  case toInit:
    ERROR ("should be done by the superserver");
    break;
  case toAccept:
    acceptServer (canBlock);
    break;
  case toRead:
    readServer (canBlock);
    break;
  case toWrite:
    writeServer (canBlock);
    break;
  case toClose:
    closeServer (canBlock);
    break;
  case toSelect:
    selectServer (canBlock);
    break;
  default:
    ERROR ("unrecognised state");
  }
}


const char *pyBotClass::getName (void)
{
  return name;
}


void pyBotClass::setName (char *botname)
{
  name = botname;
}
