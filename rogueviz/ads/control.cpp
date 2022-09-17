namespace hr {

namespace ads_game {

vector<string> move_names = { "acc down", "acc left", "acc up", "acc right", "fire", "pause", "display times", "switch spin", "menu", "[paused] future", "(paused] past", "[paused] move switch" };

void fire() {
  if(!pdata.ammo) return;
  pdata.ammo--;
  auto g = hybrid::get_where(vctr);
  auto c = g.first;
  if(g.second != 0) println(hlog, "WARNING: vctr not zeroed");

  ads_matrix S0 = ads_inverse(current * vctrV) * spin(ang*degree);

  ads_matrix S1 = S0 * lorentz(0, 2, 3); // 0.995c
  
  auto& ro = ci_at[c].rocks;
  auto r = std::make_unique<ads_object> (oMissile, c, S1, 0xC0C0FFFF);
  r->shape = &shape_missile;
  r->life_start = 0;

  ads_matrix Scell(Id, 0);    
  cell *lcell = vctr;
  auto wcell = hybrid::get_where(lcell);
  
  int steps = 0;

  compute_life(vctr, unshift(r->at), [&] (cell *c1, ld t) {
    if(true) for(int i=0; i<lcell->type; i++) {
      auto lcell1 = lcell->cmove(i);
      auto wcell1 = hybrid::get_where(lcell1);
      if(wcell1.first == c1) {
        Scell = Scell * currentmap->adj(lcell, i);
        optimize_shift(Scell);
        lcell = lcell1;
        wcell = wcell1;
        adjust_to_zero(Scell, wcell, cgi.plevel);
        steps++;
        lcell = hybrid::get_at(wcell.first, 0);
        break;
        }
      }
    if(true) if(wcell.first != c1) {
      println(hlog, "warning: got lost after ", steps, " steps");
      println(hlog, wcell);
      println(hlog, c1);
      println(hlog, "their distance is ", PIU(celldistance(wcell.first, c1)));
      return true;
      }
    auto& ci = ci_at[c1];
    hybrid::in_underlying_geometry([&] {
      gen_terrain(c1, ci);
      gen_rocks(c1, ci, 2);
      });
    if(among(ci.type, wtSolid, wtDestructible)) {
      r->life_end = t;

      auto Scell_inv = ads_inverse(Scell);
      Scell_inv = Scell_inv * r->at;
      Scell_inv = Scell_inv * ads_matrix(Id, t);
      optimize_shift(Scell_inv);

      auto X = ads_inverse(Scell);
      X = X * (r->at * ads_matrix(Id, t));
      optimize_shift(X);

      ads_matrix prel = ads_inverse(S0) * r->at * ads_matrix(Id, t);

      println(hlog, "crashed: proper time = ", t/TAU, " wall time = ", Scell_inv.shift / TAU, " player time = ", (prel.shift+ship_pt) / TAU, " start = ", ship_pt / TAU);
      if(abs(X.shift - Scell_inv.shift) > .2) {
        println(hlog, "INTRANSITIVITY ERROR! ", X.shift, " vs ", Scell_inv.shift);
        exit(1);
        }

      return true;
      }
    return false;
    });
  ro.emplace_back(std::move(r));
  }

bool handleKey(int sym, int uni) {
  /*
  if(uni == 'p') paused = !paused;
  
  if(among(uni, 'a', 'd', 's', 'w')) return true;
  
  if(uni == 't') { view_proper_times = !view_proper_times; return true; }
  if(uni == 'o') { auto_rotate = !auto_rotate; return true; }
  
  if(uni == 'f') fire();
  */

  if(sym > 0 && sym < 512 && (cmode & sm::NORMAL)) {
    char* t = multi::scfg.keyaction;
    if(t[sym] >= 16 && t[sym] < 32) return true;
    }
  
  return false;
  }

void apply_lorentz(transmatrix lor) {
  current = ads_matrix(lor, 0) * current;
  }

bool ads_turn(int idelta) {
  multi::handleInput(idelta);
  ld delta = idelta / anims::period;
  
  if(!(cmode & sm::NORMAL)) return false;
  
  hybrid::in_actual([&] {

  handle_crashes();

  auto& a = multi::actionspressed;
  auto& la = multi::lactionpressed;
  
  vector<int> ap;
  for(int i=0; i<NUMACT; i++) if(a[i]) ap.push_back(i);
  
  if(a[16+4] && !la[16+4]) fire();
  if(a[16+5] && !la[16+5]) {
    paused = !paused;
    if(paused) {
      current_ship = current;
      vctr_ship = vctr;
      vctrV_ship = vctrV;
      view_pt = 0;
      }
    else {
      current = current_ship;
      vctr = new_vctr = vctr_ship;
      vctrV = new_vctrV = vctrV_ship;
      }
    }
  if(a[16+6] && !la[16+6]) view_proper_times = !view_proper_times;
  if(a[16+7] && !la[16+7]) auto_rotate = !auto_rotate;
  if(a[16+8] && !la[16+8]) pushScreen(game_menu);    

  if(auto_angle) pconf.model_orientation += ang;

  if(true) {
    
    /* proper time passed */
    ld pt = delta * simspeed;

    bool left = a[16+1];
    bool right = a[16+3];
    bool up = a[16+2];
    bool down = a[16];
    
    int clicks = (left?1:0) + (right?1:0) + (up?1:0) + (down?1:0);

    if(left && right) left = right = false;
    if(up && down) up = down = false;
    
    if(left) ang = 180;
    if(right) ang = 0;
    if(up) ang = 90; 
    if(down) ang = 270;
    
    if(left && up) ang = 135;
    if(left && down) ang = 225;
    if(right && up) ang = 45;
    if(right && down) ang = 315;
    
    ld mul = clicks ? 1 : 0;
    if(clicks > 2) mul *= .3;
    if(!paused) {
      if(game_over || pdata.fuel < 0) mul = 0;
      }

    if(paused && a[16+11]) {
      current = ads_matrix(spin(ang*degree) * xpush(mul*delta*-5) * spin(-ang*degree), 0) * current;
      }
    else
      apply_lorentz(spin(ang*degree) * lorentz(0, 2, -delta*accel*mul) * spin(-ang*degree));
    
    if(!paused) {
      pdata.fuel -= delta*accel*mul;
      cell *c = hybrid::get_where(vctr).first;
      gen_particles(rpoisson(delta*accel*mul*20), c, ads_inverse(current * vctrV) * spin(ang*degree+M_PI) * rots::uxpush(0.06), rsrc_color[rtFuel], 0.15, 0.02);
      }

    ld tc = 0;
    if(!paused) tc = pt;
    else if(a[16+9]) tc = pt;
    else if(a[16+10]) tc = -pt;

    current.T = cspin(3, 2, tc) * current.T;
    
    optimize_shift(current);    
    hassert(eqmatrix(chg_shift(current.shift) * current.T, unshift(current)));
    
    if(auto_rotate)
      current.T = cspin(1, 0, tc) * current.T;
    else if(!paused)
      ang += tc / degree;

    if(!paused) {
      ship_pt += pt;
      pdata.oxygen -= pt;
      if(pdata.oxygen < 0) {
        pdata.oxygen = 0;
        game_over = true;
        }
      }
    else view_pt += tc;
    }

  if(auto_angle) pconf.model_orientation -= ang;
  
  fixmatrix_ads(current.T);
  fixmatrix_ads(vctrV.T);
  });

  return true;
  }

}}
