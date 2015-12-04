<?php

namespace Drupal\context\Reaction\Blocks\Form;

use Drupal\context\Plugin\ContextReaction\Blocks;
use Drupal\Core\Ajax\AjaxResponse;
use Drupal\Core\Ajax\ReplaceCommand;
use Drupal\Core\Block\BlockPluginInterface;
use Drupal\Core\Url;
use Drupal\context\ContextInterface;
use Drupal\context\Form\AjaxFormTrait;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Extension\ThemeHandlerInterface;
use Drupal\context\Reaction\ContextReactionFormBase;
use Symfony\Component\DependencyInjection\ContainerInterface;

class ConfigurationForm extends ContextReactionFormBase {

  use AjaxFormTrait;

  /**
   * @var Blocks
   */
  protected $reaction;

  /**
   * @var ThemeHandlerInterface
   */
  protected $themeHandler;

  /**
   * Construct.
   *
   * @param ThemeHandlerInterface $themeHandler
   */
  function __construct(ThemeHandlerInterface $themeHandler) {
    $this->themeHandler = $themeHandler;
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container) {
    return new static(
      $container->get('theme_handler')
    );
  }

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'context_reaction_blocks_configuration_form';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, ContextInterface $context = NULL, $reaction_id = NULL) {
    $form = parent::buildForm($form, $form_state, $context, $reaction_id);

    // Attach libraries.
    $form['#attached']['library'][] = 'block/drupal.block';

    $themes = $this->themeHandler->listInfo();

    // Select list for changing themes.
    $form['reaction']['theme'] = [
      '#type' => 'select',
      '#title' => $this->t('Theme'),
      '#options' => [],
      '#description' => $this->t('Select the theme you want to display regions for.'),
      '#ajax' => [
        'callback' => '::onThemeSelect',
      ],
    ];

    // Add each theme to the theme select.
    foreach ($themes as $theme_id => $theme) {
      $form['reaction']['theme']['#options'][$theme_id] = $theme->info['name'];
    }

    // If a theme has been selected use that to get the regions otherwise use
    // the default theme.
    $theme = $form_state->getValue(['reaction', 'theme'], $this->themeHandler->getDefault());

    // Get regions of the selected theme.
    $regions = $this->getSystemRegionList($theme);

    $form['reaction']['add'] = [
      '#type' => 'link',
      '#title' => $this->t('Place block'),
      '#attributes' => [
        'id' => 'context-reaction-blocks-region-add',
      ] + $this->getAjaxButtonAttributes(),
      '#url' => Url::fromRoute('context.reaction.blocks.library', [
        'context' => $context->id(),
        'reaction_id' => $reaction_id,
      ], [
        'query' => [
          'theme' => $theme,
        ],
      ]),
    ];

    $form['reaction']['blocks'] = [
      '#type' => 'table',
      '#header' => [
        $this->t('Block'),
        $this->t('Category'),
        $this->t('Region'),
        $this->t('Weight'),
        $this->t('Operations'),
      ],
      '#empty' => $this->t('No regions available to place blocks in.'),
      '#attributes' => [
        'id' => 'blocks',
      ],
    ];

    $blocks = $this->reaction->getBlocks()->getAllByRegion($theme);

    // Add each region.
    foreach ($regions as $region => $title) {

      // Add the tabledrag details for this region.
      $form['reaction']['blocks']['#tabledrag'][] = [
        'action' => 'match',
        'relationship' => 'sibling',
        'group' => 'block-region-select',
        'subgroup' => 'block-region-' . $region,
        'hidden' => FALSE,
      ];

      $form['reaction']['blocks']['#tabledrag'][] = [
        'action' => 'order',
        'relationship' => 'sibling',
        'group' => 'block-weight',
        'subgroup' => 'block-weight-' . $region,
      ];

      // Add the theme region.
      $form['reaction']['blocks']['region-' . $region] = [
        '#attributes' => [
          'class' => ['region-title'],
        ],
        'title' => [
          '#markup' => $title,
          '#wrapper_attributes' => [
            'colspan' => 5,
          ],
        ],
      ];

      $regionEmptyClass = empty($blocks[$region])
        ? 'region-empty'
        : 'region-populated';

      $form['reaction']['blocks']['region-' . $region . '-message'] = [
        '#attributes' => [
          'class' => ['region-message', 'region-' . $region . '-message', $regionEmptyClass],
        ],
        'message' => [
          '#markup' => '<em>' . $this->t('No blocks in this region') . '</em>',
          '#wrapper_attributes' => [
            'colspan' => 5,
          ],
        ],
      ];

      if (isset($blocks[$region])) {
        /** @var BlockPluginInterface $block */
        foreach ($blocks[$region] as $block_id => $block) {
          $configuration = $block->getConfiguration();

          $operations = [
            'edit' => [
              'title' => $this->t('Edit'),
              'url' => Url::fromRoute('context.reaction.blocks.block_edit', [
                'context' => $context->id(),
                'reaction_id' => $reaction_id,
                'block_id' => $block_id,
              ]),
            ],
          ];

          $form['reaction']['blocks'][$block_id] = [
            '#attributes' => [
              'class' => ['draggable'],
            ],
            'label' => [
              '#markup' => $block->label(),
            ],
            'category' => [
              '#markup' => $block->getPluginDefinition()['category'],
            ],
            'region' => [
              '#type' => 'select',
              '#title' => $this->t('Region for @block block', ['@block' => $block->label()]),
              '#title_display' => 'invisible',
              '#default_value' => $region,
              '#options' => $regions,
              '#attributes' => [
                'class' => ['block-region-select', 'block-region-' . $region],
              ],
            ],
            'weight' => [
              '#type' => 'weight',
              '#default_value' => isset($configuration['weight']) ? $configuration['weight'] : 0,
              '#title' => $this->t('Weight for @block block', ['@block' => $block->label()]),
              '#title_display' => 'invisible',
              '#attributes' => [
                'class' => ['block-weight', 'block-weight-' . $region],
              ],
            ],
            'operations' => [
              '#type' => 'operations',
              '#links' => $operations,
            ],
          ];
        }
      }
    }

    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $blocks = $form_state->getValue(['reaction', 'blocks']);

    /** @var BlockPluginInterface $block */
    foreach ($blocks as $blockId => $configuration) {
      $this->reaction->updateBlock($blockId, $configuration);
    }

    parent::submitForm($form, $form_state);
  }


  /**
   * Handle AJAX theme select.
   *
   * @param array $form
   *   The form array.
   *
   * @param \Drupal\Core\Form\FormStateInterface $form_state
   *   The current form state.
   *
   * @return \Drupal\Core\Ajax\AjaxResponse
   */
  public function onThemeSelect(array $form, FormStateInterface $form_state) {
    $response = new AjaxResponse();

    $response
      ->addCommand(new ReplaceCommand(
        '#context-reaction-blocks-regions', $form['reaction']['regions']
      ))
      ->addCommand(new ReplaceCommand(
        '#context-reaction-blocks-region-add', $form['reaction']['add']
      ));

    return $response;
  }

  /**
   * Get a list of regions for the select list.
   *
   * @param string $theme
   *   The theme to get a list of regions for.
   *
   * @return array
   */
  protected function getThemeRegionOptions($theme) {
    $regions = $this->getSystemRegionList($theme);

    foreach ($regions as $region => $title) {
      $regions[$region] = $title;
    }

    return $regions;
  }

  /**
   * Wraps system_region_list().
   *
   * @param string $theme
   *   The theme to get a list of regions for.
   *
   * @param string $show
   *   What type of regions that should be returned, defaults to all regions.
   *
   * @return array
   *
   * @todo This could be moved to a service since we use it in a couple of places.
   */
  protected function getSystemRegionList($theme, $show = REGIONS_ALL) {
    return system_region_list($theme, $show);
  }
}
