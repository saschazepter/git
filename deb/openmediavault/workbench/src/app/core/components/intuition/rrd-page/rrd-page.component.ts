/**
 * This file is part of OpenMediaVault.
 *
 * @license   https://www.gnu.org/licenses/gpl.html GPL Version 3
 * @author    Volker Theile <volker.theile@openmediavault.org>
 * @copyright Copyright (c) 2009-2025 Volker Theile
 *
 * OpenMediaVault is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * OpenMediaVault is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
import { Component, Inject, OnInit } from '@angular/core';
import { marker as gettext } from '@ngneat/transloco-keys-manager/marker';
import * as _ from 'lodash';
import { Observable, Subscription } from 'rxjs';
import { finalize } from 'rxjs/operators';

import { AbstractPageComponent } from '~/app/core/components/intuition/abstract-page-component';
import {
  RrdPageConfig,
  RrdPageGraphConfig
} from '~/app/core/components/intuition/models/rrd-page-config.type';
import { PageContextService, PageStatus } from '~/app/core/services/page-context.service';
import { Unsubscribe } from '~/app/decorators';
import { format, formatDeep, unixTimeStamp } from '~/app/functions.helper';
import { Icon } from '~/app/shared/enum/icon.enum';
import { BlockUiService } from '~/app/shared/services/block-ui.service';
import { DataStoreResponse, DataStoreService } from '~/app/shared/services/data-store.service';
import { RpcService } from '~/app/shared/services/rpc.service';

@Component({
  selector: 'omv-intuition-rrd-page',
  templateUrl: './rrd-page.component.html',
  styleUrls: ['./rrd-page.component.scss'],
  providers: [PageContextService]
})
export class RrdPageComponent extends AbstractPageComponent<RrdPageConfig> implements OnInit {
  @Unsubscribe()
  private subscriptions = new Subscription();

  protected monitoringEnabled = true;
  protected icon = Icon;
  protected time: number;
  protected tabs: Array<{
    label: string;
    graphs: Array<RrdPageGraphConfig>;
  }> = [];
  protected pageStatus: PageStatus;

  protected monitoringDisabledMessage: string = gettext(
    "System monitoring is disabled. To enable it, please go to the <a href='#/system/monitoring'>settings page</a>."
  );

  constructor(
    @Inject(PageContextService) pageContextService: PageContextService,
    private blockUiService: BlockUiService,
    private dataStoreService: DataStoreService,
    private rpcService: RpcService
  ) {
    super(pageContextService);
    // Check if monitoring is enabled.
    this.rpcService.request('PerfStats', 'get').subscribe((resp) => {
      this.monitoringEnabled = _.get(resp, 'enable', false);
    });
  }

  override ngOnInit(): void {
    super.ngOnInit();
    this.time = unixTimeStamp();
    this.subscriptions.add(
      this.pageContextService.status$.subscribe((status: PageStatus): void => {
        this.pageStatus = status;
      })
    );
    if (this.config?.store) {
      this.subscriptions.add(
        this.loadData().subscribe(() => {
          _.forEach(this.config.store.data, (item: Record<any, any>) => {
            const label = format(this.config.label, item);
            const graphs: RrdPageGraphConfig[] = _.map(
              this.config.graphs,
              (graph: RrdPageGraphConfig) => formatDeep(graph, item) as RrdPageGraphConfig
            );
            this.tabs.push({ label, graphs });
          });
        })
      );
    }
  }

  onGenerate() {
    this.blockUiService.start(gettext('Please wait, the statistic graphs will be regenerated ...'));
    this.rpcService
      .requestTask('Rrd', 'generate')
      .pipe(
        finalize(() => {
          this.blockUiService.stop();
        })
      )
      .subscribe(() => {
        // Force redrawing the images.
        this.time = unixTimeStamp();
      });
  }

  protected override doLoadData(): Observable<DataStoreResponse> {
    return this.dataStoreService.load(this.config.store);
  }
}
