#ifndef __HAL_HIST_H
#define __HAL_HIST_H

int hal_hist_init(void);
void hal_hist_sfp_insert(struct shw_sfp_caldata *sfp);
void hal_hist_sfp_remove(struct shw_sfp_caldata *sfp);

#endif /* __HAL_HIST_H */
