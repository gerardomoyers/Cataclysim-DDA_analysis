#pragma once
#ifndef IUSE_ACTOR_H
#define IUSE_ACTOR_H

#include <map>
#include <set>
#include <vector>

#include "calendar.h"
#include "color.h"
#include "explosion.h"
#include "game_constants.h"
#include "iuse.h"
#include "ret_val.h"
#include "string_id.h"
#include "units.h"

class vitamin;
using vitamin_id = string_id<vitamin>;
struct vehicle_prototype;
using vproto_id = string_id<vehicle_prototype>;
enum field_id : int;
enum hp_part : int;
enum body_part : int;
struct mtype;
using mtype_id = string_id<mtype>;
class JsonObject;
class Skill;
using skill_id = string_id<Skill>;
class effect_type;
using efftype_id = string_id<effect_type>;
class ammunition_type;
using ammotype = string_id<ammunition_type>;
using itype_id = std::string;
class material_type;
using material_id = string_id<material_type>;
class emit;
using emit_id = string_id<emit>;
struct bionic_data;
using bionic_id = string_id<bionic_data>;
struct furn_t;
struct itype;
class item_location;

/**
 * Transform an item into a specific type.
 * Optionally activate it.
 * Optionally split it in container and content (like opening a jar).
 *
 * It optionally checks for
 * 1. original item has a minimal amount of charges.
 * 2. player has a minimal amount of "fire" charges and consumes them,
 * 3. if fire is used, checks that the player is not underwater.
 */
class iuse_transform : public iuse_actor
{
    public:
        /** displayed if player sees transformation with %s replaced by item name */
        std::string msg_transform;

        /** type of the resulting item */
        std::string target;

        /** if set transform item to container and place new item (of type @ref target) inside */
        std::string container;

        /** if zero or positive set remaining ammo of @ref target to this (after transformation) */
        long ammo_qty = -1;

        /** if positive set transformed item active and start countdown */
        int countdown = 0;

        /** if both this and ammo_qty are specified then set @ref target to this specific ammo */
        std::string ammo_type;

        /** used to set the active property of the transformed @ref target */
        bool active = false;

        /** subtracted from @ref Creature::moves when transformation is successful */
        int moves = 0;

        /** minimum number of fire charges required (if any) for transformation */
        long need_fire = 0;

        /** displayed if item is in player possession with %s replaced by item name */
        std::string need_fire_msg;

        /** minimum charges (if any) required for transformation */
        long need_charges = 0;

        /** displayed if item is in player possession with %s replaced by item name */
        std::string need_charges_msg;

        std::string menu_text;

        iuse_transform( const std::string &type = "transform" ) : iuse_actor( type ) {}

        ~iuse_transform() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
        std::string get_name() const override;
        void finalize( const itype_id &my_item_type ) override;
        void info( const item &, std::vector<iteminfo> & ) const override;
};

class countdown_actor : public iuse_actor
{
    public:
        countdown_actor( const std::string &type = "countdown" ) : iuse_actor( type ) {}

        /** if specified overrides default action name */
        std::string name;

        /** turns before countdown action (defaults to @ref itype::countdown_interval) */
        int interval = 0;

        /** message if player sees activation with %s replaced by item name */
        std::string message;

        ~countdown_actor() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
        ret_val<bool> can_use( const player &, const item &it, bool, const tripoint & ) const override;
        std::string get_name() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;
};

/**
 * This is a @ref iuse_actor for active items that explode when
 * their charges reaches 0.
 * It can be called each turn, it can make a sound each turn.
 */
class explosion_iuse : public iuse_actor
{
    public:
        // Structure describing the explosion + shrapnel
        // Ignored if its power field is < 0
        explosion_data explosion;

        // Those 2 values are forwarded to game::draw_explosion,
        // Nothing is drawn if radius < 0 (game::explosion might still draw something)
        int draw_explosion_radius = -1;
        nc_color draw_explosion_color = c_white;
        /** Call game::flashbang? */
        bool do_flashbang = false;
        bool flashbang_player_immune = false;
        /** Create fields of this type around the center of the explosion */
        int fields_radius = -1;
        field_id fields_type;
        int fields_min_density = 1;
        int fields_max_density = MAX_FIELD_DENSITY;
        /** Calls game::emp_blast if >= 0 */
        int emp_blast_radius = -1;
        /** Calls game::scrambler_blast if >= 0 */
        int scrambler_blast_radius = -1;
        /** Volume of sound each turn, -1 means no sound at all */
        int sound_volume = -1;
        std::string sound_msg;
        /** Message shown when the player tries to deactivate the item,
         * which is not allowed. */
        std::string no_deactivate_msg;

        explosion_iuse( const std::string &type = "explosion" ) : iuse_actor( type ) {}

        ~explosion_iuse() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;
};

/**
 * This iuse creates a new vehicle on the map.
 */
class unfold_vehicle_iuse : public iuse_actor
{
    public:
        /** Id of the vehicle prototype (@see map::add_vehicle what it expects) that will be
         * created when unfolding the item. */
        vproto_id vehicle_id;
        /** Message shown after successfully unfolding the item. */
        std::string unfold_msg;
        /** Creature::moves it takes to unfold. */
        int moves = 0;
        std::map<std::string, int> tools_needed;

        unfold_vehicle_iuse( const std::string &type = "unfold_vehicle" ) : iuse_actor( type ) {}

        ~unfold_vehicle_iuse() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
};

/** Used in consume_drug_iuse for storing effect data. */
struct effect_data {
    efftype_id id;
    time_duration duration;
    body_part bp;
    bool permanent;

    effect_data( const efftype_id &nid, const time_duration &dur, body_part nbp, bool perm ) :
        id( nid ), duration( dur ), bp( nbp ), permanent( perm ) {}
};

/**
 * This iuse encapsulates the effects of taking a drug.
 */
class consume_drug_iuse : public iuse_actor
{
    public:
        /** Message to display when drug is consumed. **/
        std::string activation_message;
        /** Fields to produce when you take the drug, mostly intended for various kinds of smoke. **/
        std::map<std::string, int> fields_produced;
        /** Tool charges needed to take the drug, e.g. fire. **/
        std::map<std::string, int> charges_needed;
        /** Tools needed, but not consumed, e.g. "smoking apparatus". **/
        std::map<std::string, int> tools_needed;
        /** An effect or effects (conditions) to give the player for the stated duration. **/
        std::vector<effect_data> effects;
        /** A list of stats and adjustments to them. **/
        std::map<std::string, int> stat_adjustments;

        /** Modify player vitamin_levels by random amount between min (first) and max (second) */
        std::map<vitamin_id, std::pair<int, int>> vitamins;

        /** How many move points this action takes. */
        int moves = 100;

        consume_drug_iuse( const std::string &type = "consume_drug" ) : iuse_actor( type ) {}

        ~consume_drug_iuse() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;

        /** Item produced after using drugs. */
        std::string used_up_item;
};

/**
 * This is a @ref iuse_transform that uses the age of the item instead of a counter.
 * The age is calculated from the current turn and the birthday of the item.
 * The player has to activate the item manually, only when the specific
 * age has been reached, it will transform.
 */
class delayed_transform_iuse : public iuse_transform
{
    public:
        /**
         * The minimal age of the item (in turns) to allow the transformation.
         */
        int transform_age = 0;
        /**
         * Message to display when the user activates the item before the
         * age has been reached.
         */
        std::string not_ready_msg;

        /** How much longer (in turns) until the transformation can be done, can be negative. */
        int time_to_do( const item &it ) const;

        delayed_transform_iuse( const std::string &type = "delayed_transform" ) : iuse_transform( type ) {}

        ~delayed_transform_iuse() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
};

/**
 * This iuse contains the logic to transform a robot item into an actual monster on the map.
 */
class place_monster_iuse : public iuse_actor
{
    public:
        /** The monster type id of the monster to create. */
        mtype_id mtypeid;
        /** If true, place the monster at a random square around the player,
         * otherwise allow the player to select the target square. */
        bool place_randomly = false;
        /** How many move points this action takes. */
        int moves = 100;
        /** Difficulty of programming the monster (to be friendly). */
        int difficulty = 0;
        /** Shown when programming the monster succeeded and it's friendly. Can be empty. */
        std::string friendly_msg;
        /** Shown when programming the monster failed and it's hostile. Can be empty. */
        std::string hostile_msg;
        /** Skills used to make the monster not hostile when activated. **/
        skill_id skill1 = skill_id::NULL_ID();
        skill_id skill2 = skill_id::NULL_ID();

        place_monster_iuse() : iuse_actor( "place_monster" ) { }
        ~place_monster_iuse() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
};

/**
 * Items that can be worn and can be activated to consume energy from UPS.
 * Note that the energy consumption is done in @ref player::process_active_items, it is
 * *not* done by this class!
 */
class ups_based_armor_actor : public iuse_actor
{
    public:
        /** Shown when activated. */
        std::string activate_msg;
        /** Shown when deactivated. */
        std::string deactive_msg;
        /** Shown when it runs out of power. */
        std::string out_of_power_msg;

        ups_based_armor_actor( const std::string &type = "ups_based_armor" ) : iuse_actor( type ) {}

        ~ups_based_armor_actor() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
};

/**
 * This implements lock picking.
 */
class pick_lock_actor : public iuse_actor
{
    public:
        /**
         * How good the used tool is at picking a lock.
         */
        int pick_quality = 0;

        pick_lock_actor() : iuse_actor( "picklock" ) {}

        ~pick_lock_actor() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
};

/**
 * Implements deployable furniture from items
 */
class deploy_furn_actor : public iuse_actor
{
    public:
        /**
         * furniture type id the item should create
         */
        string_id<furn_t> furn_type;

        deploy_furn_actor() : iuse_actor( "deploy_furn" ) {}

        ~deploy_furn_actor() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
};

/**
 * Reveals specific things on the overmap.
 */
class reveal_map_actor : public iuse_actor
{
    public:
        /**
         * The radius of the overmap area that gets revealed.
         * This is in overmap terrain coordinates.
         * A radius of 1 means all terrains directly around center are revealed.
         * The center is location of nearest city defined in `reveal_map_center_omt` variable of
         * activated item (or current player global omt location if variable is not set).
         */
        int radius = 0;
        /**
         * Overmap terrain types that get revealed.
         */
        std::vector<std::string> omt_types;
        /**
         * The message displayed after revealing.
         */
        std::string message;

        void reveal_targets( const tripoint &center, const std::string &target, int reveal_distance ) const;

        reveal_map_actor( const std::string &type = "reveal_map" ) : iuse_actor( type ) {}

        ~reveal_map_actor() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
};

/**
 * Starts a fire instantly
 */
class firestarter_actor : public iuse_actor
{
    public:
        /**
         * Moves used at start of the action when starting fires with good fuel.
         */
        int moves_cost_fast = 100;

        /**
         * Total moves when starting fires with mediocre fuel.
         */
        int moves_cost_slow = 1000;

        /**
         * Does it need sunlight to be used.
         */
        bool need_sunlight = false;

        static bool prep_firestarter_use( const player &p, tripoint &pos );
        /** Player here isn't const because pyromaniacs gain a mood boost from it */
        static void resolve_firestarter_use( player &p, const tripoint &pos );
        /** Modifier on speed - higher is better, 0 means it won't work. */
        float light_mod( const tripoint &pos ) const;
        /** Checks quality of fuel on the tile and interpolates move cost based on that. */
        int moves_cost_by_fuel( const tripoint &pos ) const;

        firestarter_actor( const std::string &type = "firestarter" ) : iuse_actor( type ) {}

        ~firestarter_actor() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        ret_val<bool> can_use( const player &, const item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
};

/**
 * Cuts stuff up into components
 */
class salvage_actor : public iuse_actor
{
    public:
        /** Moves used per unit of volume of cut item */
        int moves_per_part = 25;

        /** Materials it can cut */
        std::set<material_id> material_whitelist = {
            material_id( "cotton" ),
            material_id( "leather" ),
            material_id( "fur" ),
            material_id( "faux_fur" ),
            material_id( "nomex" ),
            material_id( "kevlar" ),
            material_id( "plastic" ),
            material_id( "wood" ),
            material_id( "wool" ),
            material_id( "neoprene" )
        };

        bool try_to_cut_up( player &p, item &it ) const;
        int cut_up( player &p, item &it, item_location &cut ) const;
        int time_to_cut_up( const item &it ) const;
        bool valid_to_cut_up( const item &it ) const;

        salvage_actor( const std::string &type = "salvage" ) : iuse_actor( type ) {}

        ~salvage_actor() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
};

/**
 * Writes on stuff (ground or items)
 */
class inscribe_actor : public iuse_actor
{
    public:
        // Can it write on items/terrain
        bool on_items = true;
        bool on_terrain = false;

        // Does it require target material to be from the whitelist?
        bool material_restricted = true;

        // Materials it can write on
        std::set<material_id> material_whitelist = {
            material_id( "wood" ),
            material_id( "plastic" ),
            material_id( "glass" ),
            material_id( "chitin" ),
            material_id( "iron" ),
            material_id( "steel" ),
            material_id( "silver" ),
            material_id( "bone" )
        };

        // How will the inscription be described
        std::string verb = "Carve";
        std::string gerund = "Carved";

        bool item_inscription( item &cut ) const;

        inscribe_actor( const std::string &type = "inscribe" ) : iuse_actor( type ) {}

        ~inscribe_actor() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
};

/**
 * Cauterizes a wounded/masochistic survivor
 */
class cauterize_actor : public iuse_actor
{
    public:
        // Use flame. If false, uses item charges instead.
        bool flame = true;

        static bool cauterize_effect( player &p, item &it, bool force );

        cauterize_actor( const std::string &type = "cauterize" ) : iuse_actor( type ) {}

        ~cauterize_actor() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        ret_val<bool> can_use( const player &, const item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
};

/**
 * Makes a zombie corpse into a zombie slave
 */
class enzlave_actor : public iuse_actor
{
    public:
        enzlave_actor( const std::string &type = "enzlave" ) : iuse_actor( type ) {}

        ~enzlave_actor() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        ret_val<bool> can_use( const player &, const item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
};

/**
 * Try to turn on a burning melee weapon
 * Not iuse_transform, because they don't have that much in common
 */
class fireweapon_off_actor : public iuse_actor
{
    public:
        std::string target_id;
        std::string success_message;
        std::string lacks_fuel_message;
        std::string failure_message; // Due to bad roll
        int noise = 0; // If > 0 success message is a success sound instead
        int moves = 0;
        // Lower is better: rng(0, 10) - item.damage_level( 4 ) > this variable
        int success_chance = INT_MIN;

        fireweapon_off_actor() : iuse_actor( "fireweapon_off" ) {}

        ~fireweapon_off_actor() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        ret_val<bool> can_use( const player &, const item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
};

/**
 * Active burning melee weapon
 */
class fireweapon_on_actor : public iuse_actor
{
    public:
        std::string noise_message; // If noise is 0, message content instead
        std::string voluntary_extinguish_message;
        std::string charges_extinguish_message;
        std::string water_extinguish_message;
        std::string auto_extinguish_message;
        int noise = 0; // If 0, it produces a message instead of noise
        int noise_chance = 1; // one_in(this variable)
        int auto_extinguish_chance; // one_in(this) per turn to fail

        fireweapon_on_actor( const std::string &type = "fireweapon_on" ) : iuse_actor( type ) {}

        ~fireweapon_on_actor() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
};

/**
 * Makes noise of a given volume
 */
class manualnoise_actor : public iuse_actor
{
    public:
        std::string no_charges_message;
        std::string use_message;
        std::string noise_message;
        int noise = 0; // Should work even with no volume, even if it seems impossible
        int moves = 0;

        manualnoise_actor( const std::string &type = "manualnoise" ) : iuse_actor( type ) {}

        ~manualnoise_actor() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        ret_val<bool> can_use( const player &, const item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
};

/**
 * Plays music
 */
class musical_instrument_actor : public iuse_actor
{
    public:
        /**
         * Speed penalty when playing the instrument
         */
        int speed_penalty;
        /**
         * Volume of the music played
         */
        int volume;
        /**
         * Base morale bonus/penalty
         */
        int fun;
        /**
         * Morale bonus scaling (off current perception)
         */
        int fun_bonus;
        /**
        * List of sound descriptions for players
        */
        std::vector< std::string > player_descriptions;
        /**
        * List of sound descriptions for NPCs
        */
        std::vector< std::string > npc_descriptions;
        /**
         * Display description once per this duration (@ref calendar::once_every).
         */
        time_duration description_frequency = 0_turns;

        musical_instrument_actor( const std::string &type = "musical_instrument" ) : iuse_actor( type ) {}

        ~musical_instrument_actor() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        ret_val<bool> can_use( const player &, const item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
};

/**
 * Holster a weapon
 */
class holster_actor : public iuse_actor
{
    public:
        /** Prompt to use when selecting an item */
        std::string holster_prompt;
        /** Message to show when holstering an item */
        std::string holster_msg;
        /** Maximum volume of each item that can be holstered */
        units::volume max_volume;
        /** Minimum volume of each item that can be holstered or 1/3 max_volume if unspecified */
        units::volume min_volume;
        /** Maximum weight of each item. If unspecified no weight limit is imposed */
        units::mass max_weight = units::mass( -1, units::mass::unit_type{} );
        /** Total number of items that holster can contain **/
        int multi = 1;
        /** Base cost of accessing/storing an item. Scales down to half of that with skills. */
        int draw_cost = INVENTORY_HANDLING_PENALTY;
        /** Guns using any of these skills can be holstered */
        std::vector<skill_id> skills;
        /** Items with any of these flags set can be holstered */
        std::vector<std::string> flags;

        /** Check if obj could be stored in the holster */
        bool can_holster( const item &obj ) const;

        /** Store an object in the holster */
        bool store( player &p, item &holster, item &obj ) const;

        holster_actor( const std::string &type = "holster" ) : iuse_actor( type ) {}

        ~holster_actor() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;

        units::volume max_stored_volume() const;
};

/**
 * Store ammo and later reload using it
 */
class bandolier_actor : public iuse_actor
{
    public:
        /** Total number of rounds that can be stored **/
        int capacity = 1;

        /** What types of ammo can be stored? */
        std::set<ammotype> ammo;

        /** Base cost of accessing/storing an item. Scales down to half of that with skills. */
        int draw_cost = INVENTORY_HANDLING_PENALTY;

        /** Can this type of ammo ever be stored */
        bool is_valid_ammo_type( const itype & ) const;

        /** Check if obj could be stored in the bandolier */
        bool can_store( const item &bandolier, const item &obj ) const;

        /** Store ammo in the bandolier */
        bool reload( player &p, item &obj ) const;

        bandolier_actor( const std::string &type = "bandolier" ) : iuse_actor( type ) {}

        ~bandolier_actor() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;

        units::volume max_stored_volume() const;
};

class ammobelt_actor : public iuse_actor
{
    public:
        itype_id belt; /** what type of belt is created with this linkage? */

        ammobelt_actor() : iuse_actor( "ammobelt" ) {}

        ~ammobelt_actor() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;
};

/**
 * Repair an item
 */
class repair_item_actor : public iuse_actor
{
    public:
        /** Materials we are allowed to repair */
        std::set<material_id> materials;
        /** Skill used */
        skill_id used_skill;
        /** Maximum skill level that can be gained by using this actor. */
        int trains_skill_to;
        /**
          * Volume of materials required (and used up) as percentage of repaired item's volume.
          * Set to 0 to always use just 1 component.
          */
        float cost_scaling;
        /** Extra value added to skill roll */
        int tool_quality;
        /** Move cost for every attempt */
        int move_cost;

        enum attempt_hint : int {
            AS_SUCCESS = 0,     // Success, but can retry
            AS_RETRY,           // Failed, but can retry
            AS_FAILURE,         // Failed hard, don't retry
            AS_DESTROYED,       // Failed and destroyed item
            AS_CANT,            // Couldn't attempt
            AS_CANT_USE_TOOL,   // Cannot use tool
            AS_CANT_YET         // Skill too low
        };

        enum repair_type : int {
            RT_NOTHING = 0,
            RT_REPAIR,          // Just repairing damage
            RT_REFIT,           // Refitting
            RT_REINFORCE,       // Getting damage below 0
            RT_PRACTICE,        // Wanted to reinforce, but can't
            NUM_REPAIR_TYPES
        };

        /** Attempts to repair target item with selected tool */
        attempt_hint repair( player &pl, item &tool, item &target ) const;
        /** Checks if repairs on target item are possible. Excludes checks on tool.
          * Doesn't just estimate - should not return true if repairs are not possible or false if they are. */
        bool can_repair_target( player &pl, const item &target, bool print_msg ) const;
        /** Checks if we are allowed to use the tool. */
        bool can_use_tool( const player &p, const item &tool, bool print_msg ) const;

        /** Returns if components are available. Consumes them if `just_check` is false. */
        bool handle_components( player &pl, const item &fix, bool print_msg, bool just_check ) const;
        /** Returns the chance to repair and to damage an item. */
        std::pair<float, float> repair_chance(
            const player &pl, const item &fix, repair_type action_type ) const;
        /** What are we most likely trying to do with this item? */
        repair_type default_action( const item &fix, int current_skill_level ) const;
        /**
         * Calculates the difficulty to repair an item
         * based on recipes to craft it and player's knowledge of them.
         * If `training` is true, player's lacking knowledge and skills are not used to increase difficulty.
         */
        int repair_recipe_difficulty( const player &pl, const item &fix, bool training = false ) const;
        /** Describes members of `repair_type` enum */
        static const std::string &action_description( repair_type );

        repair_item_actor( const std::string &type = "repair_item" ) : iuse_actor( type ) {}

        ~repair_item_actor() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;

        std::string get_name() const override;
};

class heal_actor : public iuse_actor
{
    public:
        /** How much hp to restore when healing limbs? */
        float limb_power = 0;
        /** How much hp to restore when healing head? */
        float head_power = 0;
        /** How much hp to restore when healing torso? */
        float torso_power = 0;
        /** How many intensity levels will be applied when healing limbs? */
        float bandages_power = 0;
        /** Extra intensity levels gained per skill level when healing limbs. */
        float bandages_scaling = 0;
        /** How many intensity levels will be applied when healing limbs? */
        float disinfectant_power = 0;
        /** Extra intensity levels gained per skill level when healing limbs. */
        float disinfectant_scaling = 0;
        /** Chance to remove bleed effect. */
        float bleed = 0;
        /** Chance to remove bite effect. */
        float bite = 0;
        /** Chance to remove infected effect. */
        float infect = 0;
        /** Cost in moves to use the item. */
        int move_cost = 100;
        /** Is using this item a long action. */
        bool long_action = false;
        /** Extra hp gained per skill level when healing limbs. */
        float limb_scaling = 0;
        /** Extra hp gained per skill level when healing head. */
        float head_scaling = 0;
        /** Extra hp gained per skill level when healing torso. */
        float torso_scaling = 0;
        /** Effects to apply to patient on finished healing. */
        std::vector<effect_data> effects;
        /**
         * Item produced on finished healing. For example, bloody rag.
         * If the used item is a tool it, it will be turned into the used up item.
         * If it is not a tool a new item with this id will be created.
         */
        std::string used_up_item_id;
        int used_up_item_quantity = 1;
        int used_up_item_charges = 1;
        std::set<std::string> used_up_item_flags;

        /** How much hp would `healer` heal using this actor on `healed` body part. */
        int get_heal_value( const player &healer, hp_part healed ) const;
        /** How many intensity levels will be applied using this actor by `healer`. */
        int get_bandaged_level( const player &healer ) const;
        /** How many intensity levels will be applied using this actor by `healer`. */
        int get_disinfected_level( const player &healer ) const;
        /** Does the actual healing. Used by both long and short actions. Returns charges used. */
        long finish_using( player &healer, player &patient, item &it, hp_part part ) const;

        hp_part use_healing_item( player &healer, player &patient, item &it, bool force ) const;

        heal_actor( const std::string &type = "heal" ) : iuse_actor( type ) {}

        ~heal_actor() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
        void info( const item &, std::vector<iteminfo> & ) const override;
};

struct ter_t;
struct trap;
class place_trap_actor : public iuse_actor
{
    public:
        using trap_str_id = string_id<trap>;
        using ter_str_id = string_id<ter_t>;
        struct data {
            data();
            trap_str_id trap;
            /** The message shown when the trap has been set. */
            std::string done_message;
            /** Amount of practice of the "trap" skill. */
            int practice = 0;
            /** Move points that are used when placing the trap. */
            int moves = 100;
            void load( JsonObject jo );
        };
        /** Whether one can place the trap when underwater. */
        bool allow_underwater = false;
        /** Whether one can place the trap directly under the character itself. */
        bool allow_under_player = false;
        /** Whether the trap needs solid neighbor squares (e.g. for trap wire). */
        bool needs_solid_neighbor = false;
        /**
         * Contains a terrain id of the terrain that must exist in a neighbor square to allow
         * placing this trap. If empty, it is ignored. This is for example for snare traps.
         */
        ter_str_id needs_neighbor_terrain;
        /** Data that applies to unburied traps and to traps that *can * not be buried. */
        data unburied_data;
        /**
         * Contains the question asked when the player can bury the trap. Something like "Bury the trap?"
         */
        std::string bury_question;
        /** Data that applies to buried traps. */
        data buried_data;
        /**
         * The trap that makes up the outer layer of a 3x3 trap. This is not supported for buried traps!
         */
        trap_str_id outer_layer_trap;
        bool is_allowed( player &p, const tripoint &pos, const std::string &name ) const;

        place_trap_actor( const std::string &type = "place_trap" );
        ~place_trap_actor() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
};

class emit_actor : public iuse_actor
{
    public:
        std::set<emit_id> emits;
        /** If true multiplies the emits by number of charges on the item. */
        bool scale_qty = false;

        emit_actor( const std::string &type = "emit_actor" ) : iuse_actor( type ) {}
        ~emit_actor() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
        void finalize( const itype_id &my_item_type ) override;
};

class saw_barrel_actor : public iuse_actor
{
    public:
        saw_barrel_actor( const std::string &type = "saw_barrel" ) : iuse_actor( type ) {}

        void load( JsonObject &jo ) override;
        long use( player &p, item &it, bool t, const tripoint &pnt ) const override;
        iuse_actor *clone() const override;

        ret_val<bool> can_use_on( const player &p, const item &it, const item &target ) const;
};

class install_bionic_actor : public iuse_actor
{
    public:
        install_bionic_actor( const std::string &type = "install_bionic" ) : iuse_actor( type ) {}

        void load( JsonObject & ) override {}
        long use( player &p, item &it, bool t, const tripoint &pnt ) const override;
        ret_val<bool> can_use( const player &, const item &it, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
        void finalize( const itype_id &my_item_type ) override;
};

class detach_gunmods_actor : public iuse_actor
{
    public:
        detach_gunmods_actor( const std::string &type = "detach_gunmods" ) : iuse_actor( type ) {}

        void load( JsonObject & ) override {}
        long use( player &p, item &it, bool t, const tripoint &pnt ) const override;
        ret_val<bool> can_use( const player &, const item &it, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
        void finalize( const itype_id &my_item_type ) override;
};

class mutagen_actor : public iuse_actor
{
    public:
        std::string mutation_category;
        bool is_weak;
        bool is_strong;

        mutagen_actor() : iuse_actor( "mutagen" ) {}

        ~mutagen_actor() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
};

class mutagen_iv_actor : public iuse_actor
{
    public:
        std::string mutation_category;

        mutagen_iv_actor() : iuse_actor( "mutagen_iv" ) {}

        ~mutagen_iv_actor() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;
};

class deploy_tent_actor : public iuse_actor
{
    public:
        string_id<furn_t> wall;
        string_id<furn_t> floor;
        cata::optional<string_id<furn_t>> floor_center;
        string_id<furn_t> door_opened;
        string_id<furn_t> door_closed;
        int radius = 1;
        cata::optional<itype_id> broken_type;

        deploy_tent_actor() : iuse_actor( "deploy_tent" ) {}

        ~deploy_tent_actor() override = default;
        void load( JsonObject &jo ) override;
        long use( player &, item &, bool, const tripoint & ) const override;
        iuse_actor *clone() const override;

        bool check_intact( const tripoint &pos ) const;
};

#endif