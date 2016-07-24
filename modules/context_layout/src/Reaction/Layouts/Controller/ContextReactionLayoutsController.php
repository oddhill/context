<?php

namespace Drupal\context_layout\Reaction\Layouts\Controller;

use Drupal\context\ContextManager;
use Drupal\context\Reaction\Blocks\Controller\ContextReactionBlocksController;
use Drupal\Core\Ajax\AjaxResponse;
use Drupal\Core\Ajax\ReplaceCommand;
use Drupal\context\ContextInterface;
use Drupal\Core\Block\BlockManagerInterface;
use Drupal\Core\Extension\ThemeHandlerInterface;
use Drupal\Core\Plugin\Context\ContextRepositoryInterface;
use Drupal\Core\Url;
use Drupal\layout_plugin\Plugin\Layout\LayoutPluginManagerInterface;
use Symfony\Component\DependencyInjection\ContainerInterface;
use Symfony\Component\HttpFoundation\Request;

/**
 * Controller for context layout reaction routes.
 */
class ContextReactionLayoutsController extends ContextReactionBlocksController {

  /**
   * Provides an interface for the discovery and instantiation of context layouts.
   *
   * @var \Drupal\layout_plugin\Plugin\Layout\LayoutPluginManagerInterface
   */
  protected $contextLayoutManager;

  /**
   * Constructs a ContextReactionLayoutsController object.
   *
   * @param \Drupal\Core\Block\BlockManagerInterface $blockManager
   *   Provides an interface for the discovery and instantiation of block plugins.
   * @param \Drupal\Core\Plugin\Context\ContextRepositoryInterface $contextRepository
   *   Offers a global context repository.
   * @param \Drupal\Core\Extension\ThemeHandlerInterface $themeHandler
   *   Manages the list of available themes.
   * @param \Drupal\context\ContextManager $contextManager
   *   Context manager service.
   * @param \Drupal\layout_plugin\Plugin\Layout\LayoutPluginManagerInterface $contextLayoutManager
   *   Provides an interface for the discovery and instantiation of layout plugins.
   */
  public function __construct(
    BlockManagerInterface $blockManager,
    ContextRepositoryInterface $contextRepository,
    ThemeHandlerInterface $themeHandler,
    ContextManager $contextManager,
    LayoutPluginManagerInterface $contextLayoutManager
  ) {
    $this->contextLayoutManager = $contextLayoutManager;
    return parent::__construct($blockManager, $contextRepository, $themeHandler, $contextManager);
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container) {
    return new static(
      $container->get('plugin.manager.block'),
      $container->get('context.repository'),
      $container->get('theme_handler'),
      $container->get('context.manager'),
      $container->get('plugin.manager.context_layout')
    );
  }

  /**
   * Display a library of blocks that can be added to the context reaction.
   *
   * @param \Symfony\Component\HttpFoundation\Request $request
   *    The request object.
   *
   * @param \Drupal\context\ContextInterface $context
   *    The context the blocks reaction belongs to.
   *
   * @param string $reaction_id
   *    The ID of the blocks reaction that the selected block
   *    should be added to.
   *
   * @return array
   *    Build array of block library.
   */
  public function blocksLibrary(Request $request, ContextInterface $context, $reaction_id) {
    // If a theme has been defined in the query string then use this for
    // the add block link, default back to the default theme.
    $theme = $request->query->get('theme');

    // If a layout has been defined in the query string then use this for
    // the add block link, default back to the default layout.
    $fallback = TRUE;
    $layout = $request->query->get('layout', $this->contextLayoutManager->getDefaultLayout($fallback));

    // Build ContextReactionBlocksController block library.
    $build = parent::blocksLibrary($request, $context, $reaction_id);

    foreach ($build['blocks']['#rows'] as &$row) {
      // Filter block operation routes.
      $url = $row['operations']['data']['#links']['add']['url'];
      $params = $url->getRouteParameters();
      // Filter block add route.
      $new_url = Url::fromRoute('context.reaction.layouts.block_add', [
        'context' => $params['context'],
        'reaction_id' => $params['reaction_id'],
        'block_id' => $params['block_id'],
      ], [
        'query' => [
          'theme' => $theme,
          'layout' => $layout,
        ],
      ]
      );
      $row['operations']['data']['#links']['add']['url'] = $new_url;
    }

    return $build;
  }

  /**
   * Callback for the theme select list on the Context layouts reaction form.
   *
   * @param Request $request
   *    The current request.
   *
   * @param ContextInterface $context
   *    The context the block reaction is located on.
   *
   * @return \Drupal\Core\Ajax\AjaxResponse
   *    AJAX Response instance.
   */
  public function themeSelect(Request $request, ContextInterface $context) {
    // Get the context form and supply it with the layouts theme value.
    $theme = $request->request->get('reactions[layouts][theme]', '', TRUE);
    // Get the context form and supply it with the layouts layout value.
    $layout = $request->request->get('reactions[layouts][layout]', '', TRUE);
    $form = $this->contextManager->getForm($context, 'edit', [
      'reactions' => [
        'layouts' => [
          'theme' => $theme,
          'layout' => $layout,
        ],
      ],
    ]);
    $response = new AjaxResponse();
    $response->addCommand(new ReplaceCommand('#context-reactions', $form['reactions']));
    return $response;
  }

  /**
   * Callback for the theme select list on the Context layouts reaction form.
   *
   * @param Request $request
   *    The current request.
   *
   * @param ContextInterface $context
   *    The context the block reaction is located on.
   *
   * @return \Drupal\Core\Ajax\AjaxResponse
   *    AJAX Response instance.
   */
  public function layoutSelect(Request $request, ContextInterface $context) {
    // Get the context form and supply it with the layouts theme value.
    $theme = $request->request->get('reactions[layouts][theme]', '', TRUE);
    // Get the context form and supply it with the layouts layout value.
    $layout = $request->request->get('reactions[layouts][layout]', '', TRUE);
    $form = $this->contextManager->getForm($context, 'edit', [
      'reactions' => [
        'layouts' => [
          'layout' => $layout,
          'theme' => $theme,
        ],
      ],
    ]);
    $response = new AjaxResponse();
    $response->addCommand(new ReplaceCommand('#context-reactions', $form['reactions']));
    return $response;
  }

}
